#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <Vulkan/buffers_vulkan.h>

using namespace ilen::render;

VulkanBuffer::VulkanBuffer(uint32_t vulkanVersion, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice,
                    vk::CommandPool commandPool, vk::raii::Queue& graphicsQueue,
                    const IlenBufferDescriptor& descriptor, const void* initialData){
    _device = *device;
    _physicalDevice = *physicalDevice;
    _commandPool = commandPool;
    _queue = *graphicsQueue;
    _descriptor = descriptor;
    _vulkanVersion = vulkanVersion;
    
    //Creando el buffer inicial
    vk::BufferUsageFlags usageFlags = vulkan::utils::transformEngineToNativeBufferUsage(descriptor.bufferType);
    usageFlags |= vk::BufferUsageFlagBits::eTransferDst;

    createBufferInternal(descriptor.bufferSize, usageFlags, vk::MemoryPropertyFlagBits::eDeviceLocal, 
    _buffer, _memory);

    if(initialData != nullptr){
        writeDataToBuffer(initialData, descriptor.bufferSize, 0);
    }
}

VulkanBuffer::~VulkanBuffer(){
    if(_buffer != nullptr) _device.destroyBuffer(_buffer);
    if(_memory != nullptr) _device.freeMemory(_memory);
}

const IlenBufferDescriptor& VulkanBuffer::getDescriptor() const {
    return _descriptor;
}

void* VulkanBuffer::getNativeHandle() const {
    return static_cast<void*>(_buffer);
}

//Función para escribir datos en el buffer ya existente
void VulkanBuffer::writeDataToBuffer(const void* data, size_t size, size_t offset){
    if(size == 0) return;

    //Idea, crear un staggin buffer con el tamaño de los datos para copiar de la RAM a la V-RAM.
    //Luego emitir una orden de copia para colocar ese fragmento en nuestro buffer más grande.
    //Finalmente, eliminar el buffer de staging.

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingMemory;
    createBufferInternal(size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingMemory);

    //Copiar datos de la CPU a la GPU
    if(_vulkanVersion >= vk::ApiVersion13){
        vk::MemoryMapInfo mapInfo{};
        mapInfo.memory = stagingMemory;
        mapInfo.offset = 0;
        mapInfo.size = size;
        void* mappedData = _device.mapMemory2(mapInfo);
        memcpy(mappedData, data, size);

        vk::MemoryUnmapInfo unmapInfo{};
        unmapInfo.memory = stagingMemory;
        _device.unmapMemory2(unmapInfo);
    }else{
        void* mappedData = _device.mapMemory(stagingMemory, 0, size);
        memcpy(mappedData, data, size);
        _device.unmapMemory(stagingMemory);
    }

    copyBuffer(stagingBuffer, _buffer, size, 0, offset);

    _device.destroyBuffer(stagingBuffer);
    _device.freeMemory(stagingMemory);
}

void VulkanBuffer::createBufferInternal(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& memory){
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = _device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequeriments = _device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocateInfo{};
    allocateInfo.allocationSize = memRequeriments.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memRequeriments.memoryTypeBits, properties);

    memory = _device.allocateMemory(allocateInfo);
    
    _device.bindBufferMemory(buffer, memory, 0);
}

void VulkanBuffer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset){
    //Necesitamos definir un command buffer para operaciones de copia en la GPU.
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    auto _ = _device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    //Iniciamos la escritura del buffer
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);
    
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);
    commandBuffer.end();

    //Ahora hacemos submit del buffer
    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    _queue.submit(submitInfo);

    _queue.waitIdle();

    _device.freeCommandBuffers(_commandPool, commandBuffer);
}


uint32_t VulkanBuffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags){
    vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice.getMemoryProperties();
    
    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags){
            return i;
        }
    }

    ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Error al definir una memoria válida en Vulkan");
}


#endif 