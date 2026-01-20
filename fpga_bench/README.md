# FPGA_dev
Command execution:
```
g++ -O3 vck_bench.cpp -o vck_bench.exe -I$XILINX_XRT/include -L$XILINX_XRT/lib -lxrt_coreutil -pthread
./vck_bench.exe
``` 
