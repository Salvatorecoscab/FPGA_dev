#include <iostream>
#include <vector>
#include <chrono>
#include <xrt/xrt_device.h>
#include <xrt/xrt_bo.h>

int main() {
    auto device = xrt::device("0000:a8:00.1");
    
    // Usamos el binario de la DPU 8PE
    std::string xclbin_path = "/opt/xilinx/overlaybins/DPUCVDX8H/8PE/dpu_DPUCVDX8H_8PE_350M_xilinx_vck5000_gen4x8_qdma_base_2.xclbin";
    auto uuid = device.load_xclbin(xclbin_path);

    size_t data_size = 1024 * 1024; // 1MB
    
    // Crear Buffer y MAPEARLO a la memoria del Host
    auto bo_in = xrt::bo(device, data_size, xrt::bo::flags::normal, 0);
    auto bo_in_map = bo_in.map<char*>(); 

    // Warm-up (para que el driver PCIe esté listo)
    bo_in.sync(xclBOSyncDirection::XCL_BO_SYNC_BO_TO_DEVICE);

    auto start = std::chrono::high_resolution_clock::now();

    // 1. Pre-procesamiento: Escribimos directo en la memoria mapeada
    // Esto es mucho más rápido que bo.write()
    for(size_t i = 0; i < 1000; ++i) {
        bo_in_map[i] = (char)(i % 256);
    }

    // 2. Sincronización de caché (indispensable para que la FPGA vea los datos)
    bo_in.sync(xclBOSyncDirection::XCL_BO_SYNC_BO_TO_DEVICE);

    // 3. Simulación de vuelta (D2H)
    bo_in.sync(xclBOSyncDirection::XCL_BO_SYNC_BO_FROM_DEVICE);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::micro> diff = end - start;
    std::cout << "Latencia Optimizada (mmap + sync): " << diff.count() << " us" << std::endl;
    
    return 0;
}
