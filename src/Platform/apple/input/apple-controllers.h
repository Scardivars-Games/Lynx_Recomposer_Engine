#pragma once
#import <GameController/GameController.h>
#include <ilen/headers/ilen-logs.h>
#include <ilen/input/input-buffer.h>

inline void AddGameControllerNotifications() {
    
    //Notificar a la app de conexión del control
    [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification
                                                    object:nil
                                                    queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification *note){
        ILEN_LOG_P("APPLE_GAMEPADS", "Gamepad connectado %s", [[(GCController*)note.object vendorName] UTF8String]);
    }];
    
    [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification
                                            object:nil
                                            queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification *note) {
        ILEN_LOG_P("APPLE_GAMEPADS", "Gamepad desconected %s", [[(GCController*)note.object vendorName] UTF8String]);
    }];
};

inline void UpdateGamePadsApple() {
    using namespace ilen::input;
    auto& input = ilen::input::InputBuffer::Get();
    NSArray<GCController *> *controllers = [GCController controllers];
    
    for(int i = 0; i < controllers.count && i < ilen::input::MAX_CONTROLLER; i++){
        GCController *controller = controllers[i];
        GCExtendedGamepad *profile = [controller extendedGamepad];
        
        if(!profile) continue; //Este mando no cumple lo esperado
        auto& gp = input.GetGamepad(i);
        
        gp.buttons.reset();
        
        //Botones de acción
        if(profile.buttonA.isPressed) gp.buttons.set((size_t)GamepadButton::South);
        if(profile.buttonB.isPressed) gp.buttons.set((size_t)GamepadButton::West);
        if(profile.buttonX.isPressed) gp.buttons.set((size_t)GamepadButton::East);
        if(profile.buttonY.isPressed) gp.buttons.set((size_t)GamepadButton::North);
        
        //Stick analogicos
        gp.leftStickX = profile.leftThumbstick.xAxis.value;
        gp.leftStickY = profile.leftThumbstick.yAxis.value;
        gp.rightStickX = profile.rightThumbstick.xAxis.value;
        gp.rightStickY = profile.rightThumbstick.yAxis.value;

        if(gp.leftStickX != 0.0f || gp.leftStickY != 0.0f) gp.buttons.set((size_t)GamepadButton::LeftStick);
        if(gp.rightStickX != 0.0f || gp.rightStickY != 0.0f) gp.buttons.set((size_t)GamepadButton::RightStick);
        
        //Gatillo
        gp.leftTriggerValue = profile.leftTrigger.value;
        gp.rightTriggerValue = profile.rightTrigger.value;
        
        if(gp.leftTriggerValue > 0.15f) gp.buttons.set((size_t)GamepadButton::LeftTrigger);
        if(gp.rightTriggerValue > 0.15f) gp.buttons.set((size_t)GamepadButton::RightTrigger);
        
        //Dpad
        if(profile.dpad.up.isPressed) gp.buttons.set((size_t)GamepadButton::DpadUp);
        if(profile.dpad.down.isPressed) gp.buttons.set((size_t)GamepadButton::DpadDown);
        if(profile.dpad.left.isPressed) gp.buttons.set((size_t)GamepadButton::DpadLeft);
        if(profile.dpad.right.isPressed) gp.buttons.set((size_t)GamepadButton::DpadRight);
        
        //Bumpers
        if(profile.rightShoulder.isPressed) gp.buttons.set((size_t)GamepadButton::RightBumper);
        if(profile.leftShoulder.isPressed) gp.buttons.set((size_t)GamepadButton::LeftBumper);

        //Analog clicks
        if(profile.rightThumbstickButton.isPressed) gp.buttons.set((size_t)GamepadButton::RightStickClick);
        if(profile.leftThumbstickButton.isPressed) gp.buttons.set((size_t)GamepadButton::LeftStickClick);
        
        //Botones especiales
        if(profile.buttonOptions.isPressed) gp.buttons.set((size_t)GamepadButton::Select);
        if(profile.buttonMenu.isPressed) gp.buttons.set((size_t)GamepadButton::Start);
    }
};
