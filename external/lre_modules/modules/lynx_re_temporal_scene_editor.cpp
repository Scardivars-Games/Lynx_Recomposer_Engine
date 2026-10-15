// Editor provisional: [] vacío, [G] Ground, [R] RotateGround, [P] marcador de aparición del prefab.
// Casillas de 3x3; fila inferior en Y=0, grid centrado en X, Z=100.
//
// Para añadir una letra nueva al editor:
// 1. Crea un vector de posiciones como `rotatingGrounds`.
// 2. Reconoce la letra en el bloque `if (value == ...)` y guarda allí su casilla.
// 3. Limpia ese vector junto a los demás cuando el grid sea inválido.
// 4. Recorre sus posiciones y añade a `scene["entities"]` los componentes deseados.
// 5. Si se usa una escena base, decide arriba qué entidades antiguas debe reemplazar.
// 6. Añade una prueba que lea el JSON producido y compruebe sus componentes.
#include <charconv>
#include <cstdint>
#include <random>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <json/json.hpp>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {
constexpr double cellSize = 3.0;
std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::string header(const std::string& line, std::string_view key) {
    const auto value = trim(line);
    if (value.substr(0, key.size()) != key)
        throw std::runtime_error("Se esperaba la cabecera " + std::string(key));
    const auto result = trim(value.substr(key.size()));
    if (result.empty()) throw std::runtime_error("Cabecera vacía: " + std::string(key));
    return std::string(result);
}

std::size_t dimension(std::string_view value) {
    value = trim(value);
    std::size_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || result == 0)
        throw std::runtime_error("Grid_Size debe contener dos enteros positivos: ancho x alto.");
    return result;
}

bool confirm(const std::string& question) {
    std::cout << question << " [s/N]: " << std::flush;
    std::string answer;
    if (!std::getline(std::cin, answer)) return false;
    const auto value = trim(answer);
    return value == "s" || value == "S" || value == "si" || value == "sí" ||
           value == "SI" || value == "Sí";
}

// Escribir primero un temporal evita truncar el destino si falla la escritura.
void save(const fs::path& path, const std::string& contents) {
    fs::path temporary = path;
    temporary += ".tmp";
    if (fs::exists(temporary))
        throw std::runtime_error("Ya existe el archivo temporal: " + temporary.string());
    try {
        std::ofstream output(temporary, std::ios::binary);
        if (!output) throw std::runtime_error("No se pudo crear " + temporary.string());
        output << contents;
        output.close();
        if (!output) throw std::runtime_error("No se pudo escribir " + temporary.string());
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
            throw std::runtime_error("No se pudo reemplazar " + path.string());
#else
        fs::rename(temporary, path);
#endif
    } catch (...) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw;
    }
}
} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2 && !(argc == 4 && std::string_view(argv[2]) == "--base-scene")) {
        std::cerr << "Uso: LYNX_TEMPORAL_SCENE_EDITOR <nivel.txt> [--base-scene escena.meta_ilenscene]\n";
        return 1;
    }
    try {
        const fs::path inputPath(argv[1]);
        if (inputPath.extension() != ".txt")
            throw std::runtime_error("El archivo de entrada debe tener extensión .txt.");
        std::ifstream input(inputPath, std::ios::binary);
        if (!input) throw std::runtime_error("No se pudo abrir " + inputPath.string());
        std::string line;
        if (!std::getline(input, line)) throw std::runtime_error("Falta Level_Name.");
        if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        const auto name = header(line, "Level_Name:");
        if (!std::getline(input, line)) throw std::runtime_error("Falta Grid_Size.");
        const auto size = header(line, "Grid_Size:");
        const auto separator = size.find('x');
        if (separator == std::string::npos)
            throw std::runtime_error("Grid_Size debe usar el formato ancho x alto, por ejemplo 20x10.");
        const auto width = dimension(std::string_view(size).substr(0, separator));
        const auto height = dimension(std::string_view(size).substr(separator + 1));
        // Límite explícito para evitar tamaños desbordados o regeneraciones accidentales enormes.
        constexpr std::size_t maxCells = 1000000;
        if (width > maxCells || height > maxCells / width)
            throw std::runtime_error("El grid supera el límite de 1000000 casillas.");

        // Cada vector contiene pares (columna, fila). Separarlos por tipo permite
        // crear después componentes distintos para G, R y P.
        std::vector<std::pair<std::size_t, std::size_t>> grounds;
        std::vector<std::pair<std::size_t, std::size_t>> rotatingGrounds;
        std::vector<std::pair<std::size_t, std::size_t>> players;
        std::string unknownCell;
        std::size_t rows = 0;
        bool valid = true;
        while (std::getline(input, line)) {
            ++rows;
            auto row = trim(line);
            std::size_t cells = 0;
            bool malformed = false;
            while (!row.empty()) {
                if (row.front() != '[') { malformed = true; break; }
                const auto end = row.find(']');
                if (end == std::string_view::npos || row.substr(1, end - 1).find('[') != std::string_view::npos) {
                    malformed = true;
                    break;
                }
                // Extrae el texto entre corchetes. Para admitir otra letra, añade
                // aquí otra rama y recuerda incluirla en el mensaje de error.
                const auto value = trim(row.substr(1, end - 1));
                if (value == "G") grounds.emplace_back(cells, rows - 1);
                else if (value == "R") rotatingGrounds.emplace_back(cells, rows - 1);
                else if (value == "P") players.emplace_back(cells, rows - 1);
                else if (!value.empty() && unknownCell.empty())
                    unknownCell = "Valor desconocido [" + std::string(value) + "] en fila " +
                        std::to_string(rows) + ", columna " + std::to_string(cells + 1) + ". Solo se admite [], [G], [R] o [P].";
                ++cells;
                row = trim(row.substr(end + 1));
            }
            if (malformed || cells != width || rows > height) {
                valid = false;
                std::cerr << "Fila " << rows << ": " << cells << " casillas completas; se esperaban " << width;
                if (malformed) std::cerr << "; sintaxis inválida (cada casilla debe ser [...])";
                if (rows > height) std::cerr << "; fila fuera del grid";
                std::cerr << ".\n";
            }
        }
        if (input.bad()) throw std::runtime_error("Error al leer el layout.");
        input.close();
        if (rows != height) {
            valid = false;
            std::cerr << "El grid contiene " << rows << " filas; se esperaban " << height << ".\n";
        }
        if (!valid && !confirm("¿Borrar todo el layout actual del .txt y crear un grid en blanco conservando el nombre y tamaño?")) {
            std::cout << "Operación cancelada. No se modificó ningún archivo.\n";
            return 1;
        }
        if (valid && !unknownCell.empty()) throw std::runtime_error(unknownCell);
        // Al regenerar un grid inválido se vacían también las entidades que ya se
        // habían encontrado, para que la nueva escena coincida con el grid blanco.
        if (!valid) { grounds.clear(); rotatingGrounds.clear(); players.clear(); }
        if (players.size() > 1)
            throw std::runtime_error("Solo se permite un marcador [P] por nivel.");

        nlohmann::json scene = {{"scene_name", name}, {"entities", nlohmann::json::array()}};
        if (argc == 4) {
            std::ifstream baseFile(argv[3]);
            if (!baseFile) throw std::runtime_error("No se pudo abrir la escena base.");
            baseFile >> scene;
            if (!scene.is_object() || !scene.contains("entities") || !scene["entities"].is_array())
                throw std::runtime_error("La escena base debe contener una lista entities.");
            // Una escena base puede contener cámaras, decorado u otras entidades.
            // `retained` conserva lo que no sea responsabilidad de este layout.
            auto retained = nlohmann::json::array();
            for (const auto& entity : scene["entities"]) {
                if (!entity.is_object() || !entity.contains("components") || !entity["components"].is_object())
                    throw std::runtime_error("Entidad inválida en la escena base.");
                const auto& components = entity["components"];
                if (components.contains("ScriptingBridgeComponent")) {
                    const auto script = components["ScriptingBridgeComponent"].value("scriptName", "");
                    // `continue` no copia estos objetos: las casillas G y R del
                    // archivo de texto crearán sus versiones nuevas más abajo.
                    if (script == "ilen::game::Ground" || script == "ilen::game::RotateGround") continue;
                }
                if (!players.empty()) {
                    const auto script = components.contains("ScriptingBridgeComponent")
                        ? components["ScriptingBridgeComponent"].value("scriptName", "") : "";
                    if (script == "ilen::game::Cube" || script == "ilen::game::GravityCamera" ||
                        (components.contains("CameraComponent") && components["CameraComponent"].value("isPrimary", false))) continue;
                }
                retained.push_back(entity);
            }
            scene["entities"] = std::move(retained);
            scene["scene_name"] = name;
        }
        // IDs nuevos también para las copias, evitando colisiones con la escena base.
        std::mt19937_64 random(std::random_device{}());
        std::unordered_set<std::uint64_t> used;
        auto uuid = [&]() {
            std::uint64_t id;
            do { id = random(); } while (id == 0 || !used.insert(id).second);
            return id;
        };
        int actualGround = 0;
        for (auto& entity : scene["entities"])
            entity["components"]["EntityMetadataComponent"]["uuid"] = uuid();

        // Convierte cada G del grid en una entidad del JSON de escena.
        for (const auto& [column, row] : grounds) {
            // X resta la mitad del ancho para centrar horizontalmente el grid.
            // Y invierte las filas: la primera línea del TXT es la fila superior.
            // Multiplicar por cellSize convierte índices de casilla en unidades del mundo.
            const double x = (static_cast<double>(column) - (static_cast<double>(width) - 1.0) / 2.0) * cellSize;
            const double y = (static_cast<double>(height) - 1.0 - static_cast<double>(row)) * cellSize;
            std::string finalName = "Tierra_" + std::to_string(actualGround);
            scene["entities"].push_back({{"components", {
                // Metadata: identidad y filtros generales de la entidad.
                {"EntityMetadataComponent", {{"uuid", uuid()}, {"name", finalName}, {"tags", 1}, {"layers", 1}, {"activeStatus", true}}},
                // Transform: posición y tamaño dentro del mundo.
                {"TransformComponent", {{"position", {x, y, 100.0}}, {"scale", {cellSize, cellSize, cellSize}}}},
                // Render: textura y sprite que se dibujarán.
                {"RenderComponent", {{"mainTexID", 16}, {"defaultSpriteInTextureID", 3}}},
                // Rigidbody estático: participa en colisiones, pero la física no lo mueve.
                {"Rigidbody2D", {{"gravityScale", 1.0}, {"isStatic", true}}},
                // Un objeto vacío usa los valores predeterminados del collider de caja.
                {"BoxCollider2D", nlohmann::json::object()}
                //{"ScriptingBridgeComponent", {{"scriptName", "ilen::game::Ground"}}}
            }}});
            actualGround++;
        }

        // R se construye igual que G. Sus dos diferencias están señaladas abajo:
        // el rigidbody es dinámico y la entidad ejecuta RotateGround.
        // Este bloque es un buen punto de partida para crear otro tipo sencillo.
        for (const auto& [column, row] : rotatingGrounds) {
            const double x = (static_cast<double>(column) - (static_cast<double>(width) - 1.0) / 2.0) * cellSize;
            const double y = (static_cast<double>(height) - 1.0 - static_cast<double>(row)) * cellSize;
            std::string finalName = "Tierra_" + std::to_string(actualGround);
            scene["entities"].push_back({{"components", {
                {"EntityMetadataComponent", {{"uuid", uuid()}, {"name", finalName}, {"tags", 1}, {"layers", 1}, {"activeStatus", true}}},
                {"TransformComponent", {{"position", {x, y, 100.0}}, {"scale", {cellSize, cellSize, cellSize}}}},
                {"RenderComponent", {{"mainTexID", 16}, {"defaultSpriteInTextureID", 3}}},
                // false permite que el sistema físico actualice esta entidad.
                {"Rigidbody2D", {{"gravityScale", 0.0f}, {"isStatic", false}}},
                {"BoxCollider2D", nlohmann::json::object()},
                // ScriptingBridgeComponent conecta la entidad con una clase de juego.
                {"ScriptingBridgeComponent", {{"scriptName", "ilen::game::RotateGround"}}}
            }}});
            actualGround++;
        }
        for (const auto& [column, row] : players) {
            const double x = (static_cast<double>(column) - (static_cast<double>(width) - 1.0) / 2.0) * cellSize;
            const double y = (static_cast<double>(height) - 1.0 - static_cast<double>(row)) * cellSize;
            // El script se encargará de instanciar el prefab en esta posición.
            scene["entities"].push_back({{"components", {
                {"EntityMetadataComponent", {{"uuid", uuid()}, {"name", "Player Spawn"}, {"tags", 0}, {"layers", 0}, {"activeStatus", true}}},
                {"TransformComponent", {{"position", {x, y, 10.0}}}},
                {"ScriptingBridgeComponent", {{"scriptName", "ilen::game::PlayerSpawner"}}}
            }}});
        }
        // Serializar antes de modificar archivos: una escena inválida no borra el layout.
        const auto serialized = scene.dump(4) + "\n";
        auto scenePath = inputPath;
        scenePath.replace_extension(".meta_ilenscene");
        if (fs::exists(scenePath) && !confirm("Ya existe " + scenePath.string() + ". ¿Reemplazarlo por la escena generada?")) {
            std::cout << "Operación cancelada. No se modificó ningún archivo.\n";
            return 1;
        }
        if (!valid) {
            std::string blank = "Level_Name: " + name + "\nGrid_Size: " + std::to_string(width) + "x" + std::to_string(height) + "\n";
            for (std::size_t y = 0; y < height; ++y) {
                for (std::size_t x = 0; x < width; ++x) blank += "[]";
                blank += '\n';
            }
            save(inputPath, blank);
            std::cout << "Grid en blanco guardado en " << inputPath << ".\n";
        }
        save(scenePath, serialized);
        std::cout << "Escena creada en " << scenePath << ". Grounds generados: " << grounds.size()
                  << ". RotateGrounds generados: " << rotatingGrounds.size()
                  << ". Marcadores de player generados: " << players.size() << ".\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
