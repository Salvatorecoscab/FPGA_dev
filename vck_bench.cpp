#include <iostream>
#include <vector>
#include <chrono>
#include <xrt/xrt_device.h>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>

int main() {
    // 1. Inicializar dispositivo (usa el ID que confirmaste antes)
    auto device = xrt::device("0000:a8:00.1");
    
    // 2. Cargar el binario de la DPU (8 Motores de procesamiento)
    // Este binario ya tiene el hardware listo para operaciones de tensores/stencil
    std::string xclbin_path = "/opt/xilinx/overlaybins/DPUCVDX8H/8PE/dpu_DPUCVDX8H_8PE_350M_xilinx_vck5000_gen4x8_qdma_base_2.xclbin";
    auto uuid = device.load_xclbin(xclbin_path);

    // 3. Preparar los datos (Simulando un pequeño set de análisis de datos)
    size_t data_size = 1024 * 1024; // 1MB de datos de prueba
    auto bo_in = xrt::bo(device, data_size, xrt::bo::flags::normal, 0);
    auto bo_out = xrt::bo(device, data_size, xrt::bo::flags::normal, 0);

    std::vector<char> host_data(data_size, 0xAB);

    // --- INICIO DE LA PRUEBA DE LATENCIA ---
    auto start = std::chrono::high_resolution_clock::now();

    // ETAPA 1: Pre-procesamiento en CPU (Simulado)
    for(size_t i = 0; i < 1000; ++i) host_data[i] ^= 0xFF; 

    // ETAPA 2: Transferencia a FPGA
    bo_in.write(host_data.data());
    bo_in.sync(xclBOSyncDirection::XCL_BO_SYNC_BO_TO_DEVICE);

    // ETAPA 3: Ejecución (Aquí llamarías a la DPU)
    // Nota: Como no tenemos el .xmodel, mediremos la velocidad del bus y la respuesta
    // de sincronización del hardware, que es el "piso" de latencia.

    bo_out.sync(xclBOSyncDirection::XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_out.read(host_data.data());

    auto end = std::chrono::high_resolution_clock::now();
    // --- FIN DE LA PRUEBA ---

    std::chrono::duration<double, std::micro> diff = end - start;
    std::cout << "Latencia total (Pre-proc + H2D + Sync + D2H): " << diff.count() << " us" << std::endl;
    
    return 0;
}
