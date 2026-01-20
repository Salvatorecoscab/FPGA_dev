#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <string>
#include <cstring>
#include <xrt/xrt_device.h>
#include <xrt/xrt_bo.h>

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    std::string csv_file = argv[1];

    auto device = xrt::device("0000:a8:00.1");
    auto uuid = device.load_xclbin("/opt/xilinx/overlaybins/DPUCVDX8H/8PE/dpu_DPUCVDX8H_8PE_350M_xilinx_vck5000_gen4x8_qdma_base_2.xclbin");

    // Buffer de 1GB para no quedarnos cortos
    size_t buffer_size = 1024LL * 1024 * 1024; 
    auto bo_in = xrt::bo(device, buffer_size, xrt::bo::flags::normal, 0);
    char* bo_map = bo_in.map<char*>();

    std::ifstream file(csv_file);
    std::string line;
    size_t total_bytes = 0;
    
    std::cout << "Analizando " << csv_file << "..." << std::endl;

    // --- AQUÍ EMPIEZA EL DUELO ---
    auto start = std::chrono::high_resolution_clock::now();

    while (std::getline(file, line)) {
        // Simulación de Stencil: Filtrar solo registros de un año específico
        // Esto en GPU requiere cargar TODO y luego filtrar. Aquí filtramos EN CAMINO.
        if (line.find("2019") != std::string::npos) { 
            size_t len = line.length();
            if (total_bytes + len + 1 < buffer_size) {
                std::memcpy(bo_map + total_bytes, line.c_str(), len);
                total_bytes += len;
            }
        }
    }
    
    // Sincronización final (Avisar a la FPGA que los datos están ahí)
    bo_in.sync(xclBOSyncDirection::XCL_BO_SYNC_BO_TO_DEVICE);

    auto end = std::chrono::high_resolution_clock::now();
    // --- AQUÍ TERMINA ---

    std::chrono::duration<double> diff = end - start;
    std::cout << "RESULTADO:" << std::endl;
    std::cout << "- Tiempo total (Parsing + Ingesta): " << diff.count() << " s" << std::endl;
    std::cout << "- Velocidad: " << (total_bytes / (1024.0 * 1024.0)) / diff.count() << " MB/s" << std::endl;

    return 0;
}
