#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cuda_runtime.h>

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    std::string csv_file = argv[1];

    // 1. Reservar memoria en la GPU (512MB como en la FPGA)
    size_t buffer_size = 512LL * 1024 * 1024;
    char* d_buffer;
    cudaMalloc(&d_buffer, buffer_size);

    // Buffer temporal en el Host (CPU) para el filtrado
    std::vector<char> h_buffer;
    h_buffer.reserve(buffer_size);

    std::cout << "Iniciando procesamiento en H100: " << csv_file << std::endl;

    std::ifstream file(csv_file);
    std::string line;
    int lineas_procesadas = 0;

    auto start = std::chrono::high_resolution_clock::now();

    // 2. Lectura y Filtrado (Pre-procesamiento en CPU)
    while (std::getline(file, line)) {
        if (line.find(",2019,") != std::string::npos) {
            h_buffer.insert(h_buffer.end(), line.begin(), line.end());
            h_buffer.push_back('\n');
        }
        lineas_procesadas++;
        if (lineas_procesadas % 100000 == 0) std::cout << "." << std::flush;
    }

    // 3. Transferencia a la GPU (El cuello de botella de la GPU)
    cudaMemcpy(d_buffer, h_buffer.data(), h_buffer.size(), cudaMemcpyHostToDevice);

    auto end = std::chrono::high_resolution_clock::now();

    // 4. Resultados
    std::chrono::duration<double> diff = end - start;
    double mb_procesados = h_buffer.size() / (1024.0 * 1024.0);

    std::cout << "\n\n=== RESULTADOS NVIDIA H100 ===" << std::endl;
    std::cout << "Tiempo total: " << diff.count() << " segundos" << std::endl;
    std::cout << "Datos filtrados: " << mb_procesados << " MB" << std::endl;
    std::cout << "Velocidad efectiva: " << mb_procesados / diff.count() << " MB/s" << std::endl;
    std::cout << "Latencia promedio por línea: " << (diff.count() * 1e6) / lineas_procesadas << " microsegundos" << std::endl;

    cudaFree(d_buffer);
    return 0;
}
