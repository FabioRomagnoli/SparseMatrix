#include <chrono>
#include "matrix.hpp"  

using namespace std::chrono;

int main() {
    Matrix<double, Order::ROW> matrix("lnsp_131.mtx");

    // vector of the same size as the matrix cols
    std::vector<double> v(matrix.ncols, 1.0); 

    // multiplication in uncompressed format
    auto start = high_resolution_clock::now();
    auto result_uncompressed = matrix * v;
    auto stop = high_resolution_clock::now();
    auto duration_uncompressed = duration_cast<microseconds>(stop - start);
    std::cout << "Time taken by uncompressed multiplication: "
            << duration_uncompressed.count() << " microseconds" << std::endl;

    //multiplication in compressed format
    start = high_resolution_clock::now();
    auto result_compressed = matrix * v;
    stop = high_resolution_clock::now();
    auto duration_compressed = duration_cast<microseconds>(stop - start);
    std::cout << "Time taken by compressed multiplication: "
            << duration_compressed.count() << " microseconds" << std::endl;

}
