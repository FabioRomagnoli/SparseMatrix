#include <array>
#include <vector>
#include <map>
#include <stdexcept>  
#include <iostream>   
#include <cstdlib>    
#include <iostream>
#include <iomanip> 
#include <fstream>
#include <sstream>
#include <string>
#include <complex>

enum class Order {ROW,COL};


template <typename T = double, Order O = Order::ROW>
class Matrix {
public:
    using size_type = std::size_t;
    using value_type = T;
    using key_type = std::array<size_t, 2>;
    using complex_type = std::complex<T>;

	Matrix() : nrows(0), ncols(0), isCompressed(false) {}

	Matrix(const std::string& filename): isCompressed(false) {
        matrix_market_reader(filename);
    }

    Matrix(size_type rows, size_type cols, bool compressed = false)
        : nrows(rows), ncols(cols), nnz(0), isCompressed(compressed){};


    struct KeyCompare {
        bool operator()(const key_type& lhs, const key_type& rhs) const {
            if constexpr (O == Order::COL) {
                // First sort by column index, then by row index
                if (lhs[1] == rhs[1]) {  
                    return lhs[0] < rhs[0]; 
                }
                return lhs[1] < rhs[1];  
            } else if constexpr (O == Order::ROW) {
                // First sort by row index, then by column index
                if (lhs[0] == rhs[0]) {  
                    return lhs[1] < rhs[1]; 
                }
                return lhs[0] < rhs[0]; 
            }
        }
    };


    void insert_element(size_type row, size_type col, value_type x);
    void compress();
    void uncompress();
    bool is_compressed() const;

    const T& operator()(size_type r, size_type c) const;
    T& operator()(size_type r, size_type c);


	template<typename U, Order S>
	friend std::vector<U> operator*(Matrix<U, S> const &m,
							std::vector<U> const &v);

	void vecMultiply(const std::vector<T> &v, std::vector<T> &res) const;

	void print() const;


    void matrix_market_reader(const std::string& filename);

    size_type nrows;
    size_type ncols;
    size_type nnz;	
private:

	//map to handle the matrix in an uncompressed format
    std::map<key_type, T, KeyCompare> m_matrix;

	bool isCompressed;
	//three vectors required to store the matrix in compressed sparse row/column
    std::vector<T> values;		//all the Non-Zero values
    std::vector<size_type> col_idxs;
    std::vector<size_type> row_idxs;

		//number of non-zero values

};


// class templates and function templates defintions //



template<typename T, Order O>
const T& Matrix<T,O>::operator()(size_type row, size_type col) const {
    if (!is_compressed()) {
        auto it = m_matrix.find({row, col});
        if (it != m_matrix.end()) {
            return it->second;
        } else {
			throw std::runtime_error("Values that is being queried doesn't exist in the matrix");
		}
    } else {
		if constexpr (O == Order::ROW){
			size_type upBound = row_idxs[row + 1];
			size_type lowBound = row_idxs[row];
			for(auto i = lowBound; i < upBound; ++i){
				if(col_idxs[i] == col){
					return values[i];
				}
			}
		} else {
			size_type upBound = col_idxs[col + 1];
			size_type lowBound = col_idxs[col];
			for(auto i = lowBound; i < upBound; ++i){
				if(row_idxs[i] == row){
					return values[i];
				}
			}
		}
	}
}

template<typename T, Order O>
T& Matrix<T,O>::operator()(size_type row, size_type col) {
  	if (!is_compressed()) {
        auto it = m_matrix.find({row, col});
        if (it != m_matrix.end()) {
            return it->second;
        } else {
			throw std::runtime_error("Values that is being queried doesn't exist in the matrix");
		}
    } else {
		if constexpr (O == Order::ROW){
			size_type upBound = row_idxs[row + 1];
			size_type lowBound = row_idxs[row];
			for(auto i = lowBound; i < upBound; ++i){
				if(col_idxs[i] == col){
					return values[i];
				}
			}
		} else {
			size_type upBound = col_idxs[col + 1];
			size_type lowBound = col_idxs[col];
			for(auto i = lowBound; i < upBound; ++i){
				if(row_idxs[i] == row){
					return values[i];
				}
			}
		}
	}
}

template<typename T, Order O>
void Matrix<T, O>::insert_element(size_type row, size_type col, value_type x) {
    if (!is_compressed()) {
        auto inserted = m_matrix.insert({{row, col}, x});
        if (!inserted.second) {
            inserted.first->second = x;
        } else {
            // increment if a new element was actually inserted
            ++nnz;
        }
        // Update nrows and ncols based on the new inserted element
        if (row >= nrows) nrows = row + 1;
        if (col >= ncols) ncols = col + 1;
    } else {
        if constexpr (O == Order::ROW) {
            size_type upBound = row_idxs[row + 1];
            size_type lowBound = row_idxs[row];
            bool found = false;
            for (auto i = lowBound; i < upBound; ++i) {
                if (col_idxs[i] == col) {
                    values[i] = x; 
                    break;
                }
            }
            throw std::runtime_error("Can't add a value to a compressed matrix");
        } else if constexpr (O == Order::COL) {
            size_type upBound = col_idxs[col + 1];
            size_type lowBound = col_idxs[col];
            bool found = false;
            for (auto i = lowBound; i < upBound; ++i) {
                if (row_idxs[i] == row) {
                    values[i] = x; 
                    break;
                }
            }
            throw std::runtime_error("Can't add a value to a compressed matrix");
        }
    }
}


template<typename T, Order O>
bool Matrix<T, O>::is_compressed() const {
	return isCompressed;
}

template <typename T, Order O>
std::vector<T> operator*(const Matrix<T, O>& m, const std::vector<T>& v) {
    if (v.size() != (O == Order::ROW ? m.ncols : m.nrows)) {
        throw std::runtime_error("Vector dimensions must match matrix dimensions for multiplication.");
    }
    std::vector<T> result(O == Order::ROW ? m.nrows : m.ncols, T(0));
    m.vecMultiply(v, result);
    return result;
}

template <typename T, Order O>
void Matrix<T, O>::vecMultiply(const std::vector<T>& v, std::vector<T>& res) const {
    if (is_compressed()) {
        if constexpr (O == Order::ROW) {
            // CSR (Compressed Sparse Row)
            for (size_type i = 0; i < nrows; i++) {
                for (size_type j = row_idxs[i]; j < row_idxs[i + 1]; j++) {
                    res[i] += values[j] * v[col_idxs[j]];
                }
            }
        } else if constexpr (O == Order::COL) {
            // CSC (Compressed Sparse Column)
            for (size_type j = 0; j < ncols; j++) {
                for (size_type i = col_idxs[j]; i < col_idxs[j + 1]; i++) {
                    res[row_idxs[i]] += values[i] * v[j];
                }
            }
        }
    } else {
        for (const auto& [key, value] : m_matrix) {
            res[key[0]] += value * v[key[1]];
        }
    }
}


template <typename T, Order O>
void Matrix<T, O>::compress() {
	if (is_compressed()) return;

	if constexpr (O == Order::ROW) {
		row_idxs.clear();
        row_idxs.push_back(0);
        for (size_type i = 0; i < nrows; ++i) {
            for (size_type j = 0; j < ncols; ++j) {
                auto it = m_matrix.find({i, j});
                if (it != m_matrix.end()) {
                    values.push_back(it->second);
                    col_idxs.push_back(j);
                }
            }
            row_idxs.push_back(values.size());
        }
	} else if constexpr (O == Order::COL) {
        col_idxs.clear();
        col_idxs.push_back(0);
        for (size_type j = 0; j < ncols; ++j) {
            for (size_type i = 0; i < nrows; ++i) {
                auto it = m_matrix.find({i, j});
                if (it != m_matrix.end()) {
                    values.push_back(it->second);
                    row_idxs.push_back(i);
                }
            }
            col_idxs.push_back(values.size());
        }
	}
	isCompressed = true;
	m_matrix.clear();
}

template <typename T, Order O>
void Matrix<T, O>::uncompress() {
	if (!m_matrix.empty()) return;

	if constexpr (O == Order::ROW) {
		for (size_type i = 0; i < row_idxs.size() - 1; ++i) {
			for (size_type j = row_idxs[i]; j < row_idxs[i + 1]; ++j) {
				m_matrix[{i, col_idxs[j]}] = values[j];
			}
		}
	} else if constexpr (O == Order::COL) {
		for (size_type i = 0; i < col_idxs.size() - 1; ++i) {
			for (size_type j = col_idxs[i]; j < col_idxs[i + 1]; ++j) {
				m_matrix[{row_idxs[j], i}] = values[j];
			}
		}
	}
	isCompressed = false;
	values.clear();
	col_idxs.clear();
	row_idxs.clear();
}

template <typename T, Order O>
void Matrix<T, O>::print() const {
    if (is_compressed()) {
 		std::cout << "Compressed Matrix (" << ((O == Order::ROW) ? "Row-major" : "Column-major") << "), Rows: " << nrows << ", Cols: " << ncols << std::endl;
        if constexpr (O == Order::ROW) {
            // Print the matrix in row-major order
            for (size_t i = 0; i < row_idxs.size() - 1; ++i) {
                std::cout << "Row " << i << ": ";
                for (size_t j = row_idxs[i]; j < row_idxs[i + 1]; ++j) {
                    std::cout << "(" << col_idxs[j] << ", " << values[j] << ") ";
                }
                std::cout << std::endl;
            }
        } else if constexpr (O == Order::COL) {
            // Print the matrix in column-major order (CSC format)
            for (size_t j = 0; j < ncols; ++j) {
                std::cout << "Column " << j << ": ";
                for (size_t i = col_idxs[j]; i < col_idxs[j + 1]; ++i) {
                    std::cout << "(" << row_idxs[i] << ", " << values[i] << ") ";
                }
                std::cout << std::endl;
            }
        }
    } else {
        std::cout << "Uncompressed Matrix, Rows: " << nrows << ", Cols: " << ncols << std::endl;
        for (size_t i = 0; i < nrows; ++i) {
            for (size_t j = 0; j < ncols; ++j) {
                auto it = m_matrix.find({i, j});
                if (it != m_matrix.end()) {
                    std::cout << std::setw(5) << it->second << " ";
                } else {
                    std::cout << std::setw(5) << "0 ";
                }
            }
            std::cout << std::endl;
        }
    } 
}


template <typename T, Order O>
void Matrix<T, O>::matrix_market_reader(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    size_type m, n, l;

    while (std::getline(file, line)) {
        if (line[0] != '%') break;
    }

    std::istringstream iss(line);
    iss >> m >> n >> l;

    m_matrix.clear();
    values.clear();
    col_idxs.clear();
    row_idxs.clear();
    nrows = m;
    ncols = n;
    nnz = 0;

    while (l--) {
        size_type i, j;
        T val;
        file >> i >> j >> val;
        --i; --j;
        insert_element(i, j, val);
    }

    file.close();
}
