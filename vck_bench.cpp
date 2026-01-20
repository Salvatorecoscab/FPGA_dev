#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <string>
#include <cstring>
#include <xrt/xrt_device.h>
#include <xrt/xrt_bo.h>

/**
 * Este código simula un pipeline de análisis de datos:
 * 1. Lee un CSV masivo (Censo 2019).
 * 2. Filtra filas en tiempo real (Pre-procesamiento/Stencil lógico).
 * 3. Escribe los resultados DIRECTAMENTE en la memoria de la FPGA (Zero-copy).
 */

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Uso: ./vck_bench.exe <ruta_al_archivo_csv>" << std::endl;
        return 1;
    }

    std::string csv_file = argv[1];

    // 1. Inicializar dispositivo VCK5000
    auto device = xrt::device("0000:a8:00.1");
    
    // Cargar el binario de la DPU 8PE (que es el que tenemos disponible)
    auto uuid = device.load_xclbin("/opt/xilinx/overlaybins/DPUCVDX8H/8PE/dpu_DPUCVDX8H_8PE_350M_xilinx_vck5000_gen4x8_qdma_base_2.xclbin");

    // 2. Preparar un buffer de 512MB en la FPGA para recibir los datos procesados
    size_t buffer_size = 512LL * 1024 * 1024; 
    auto bo_in = xrt::bo(device, buffer_size, xrt::bo::flags::normal, 0);
    char* bo_map = bo_in.map<char*>(); // Mapeo directo de memoria

    std::cout << "Iniciando procesamiento de: " << csv_file << std::endl;

    // 3. Procesamiento y Carga (El corazón de la comparación)
    std::ifstream file(csv_file);
    std::string line;
    size_t bytes_escritos = 0;
    int lineas_procesadas = 0;

    auto start = std::chrono::high_resolution_clock::now();

    // Leemos el CSV línea a línea
    while (std::getline(file, line)) {
        // SIMULACIÓN DE FILTRADO (Pre-procesamiento):
        // Solo enviamos a la FPGA si la línea contiene ciertos criterios (ej. un estado o código)
        // Esto en una GPU requeriría un paso extra de CPU. Aquí es "al vuelo".
        if (line.find(",2019,") != std::string::npos) { 
            size_t len = line.length();
            if (bytes_escritos + len + 1 < buffer_size) {
                // Escribimos directamente en la memoria que la FPGA ya está viendo
                std::memcpy(bo_map + bytes_escritos, line.c_str(), len);
                bo_map[bytes_escritos + len] = '\n';
                bytes_escritos += len + 1;
            }
        }
        lineas_procesadas++;
        if (lineas_procesadas % 100000 == 0) std::cout << "." << std::flush;
    }

    // Sincronizar para asegurar que la FPGA vea los datos finales
    bo_in.sync(xclBOSyncDirection::XCL_BO_SYNC_BO_TO_DEVICE);

    auto end = std::chrono::high_resolution_clock::now();

    // 4. Resultados
    std::chrono::duration<double> diff = end - start;
    double mb_procesados = bytes_escritos / (1024.0 * 1024.0);
    
    std::cout << "\n\n=== RESULTADOS VCK5000 ===" << std::endl;
    std::cout << "Tiempo total: " << diff.count() << " segundos" << std::endl;
    std::cout << "Datos útiles filtrados y cargados: " << mb_procesados << " MB" << std::endl;
    std::cout << "Velocidad efectiva de ingesta: " << mb_procesados / diff.count() << " MB/s" << std::endl;
    std::cout << "Latencia promedio por línea: " << (diff.count() * 1e6) / lineas_procesadas << " microsegundos" << std::endl;

    return 0;
}
