// namespace algebra 
// {

#include <array>
#include <vector>
#include <map>
#include <stdexcept>  // For std::runtime_error
#include <iostream>   // For std::cerr
#include <cstdlib>    // For std::exit

enum class Order {ROW,COL};


template <typename T = double, Order O = Order::ROW>
class Matrix {
public:
    using size_type = std::size_t;
    using value_type = T;
    using key_type = std::array<size_t, 2>;

	Matrix () = default; 
    Matrix(size_type row, size_type col) : nrow(row), ncol(col), nnz(0) {}

	// void resize(size_type row, size_type col);


    struct KeyCompare {
        bool operator()(const key_type& lhs, const key_type& rhs) const {
            if constexpr (O == Order::COL) {
                if (lhs[1] == rhs[1]) {
                    return lhs[0] < rhs[0];
                }
                return lhs[1] < rhs[1];
            } else {
                return lhs < rhs;
            }
        }
    };

    void insert_element(size_type row, size_type col, value_type x);
    void compress();
    void uncompress();
    bool is_compressed() const;

    const T& operator()(size_type r, size_type c) const;
    T& operator()(size_type r, size_type c);


	friend std::vector<T> operator*(Matrix<T, O> const &m,
							std::vector<T> const &v);

	void vecMultiply(const std::vector<T> &v, std::vector<T> &res) const;


	//have to implement these still, each one for both row and col ordered
	// std::vector<T> col(size_type i) const;
	// std::vector<T> row(size_type i) const;
	// void replaceCol(size_type i, std::vector<T> const &);
	// void replaceRow(size_type i, std::vector<T> const &);


	//placeholder, decide what the method should do; 
	// void matrix_market_reader();

private:

	//map to handle the matrix in an uncompressed format
    std::map<key_type, T, KeyCompare> m_matrix;

	//three vectors required to store the matrix in compressed sparse row/column
    std::vector<T> v;		//all the Non-Zero values
    std::vector<unsigned int> col_idxs;
    std::vector<unsigned int> row_idxs;
    std::vector<size_type> idx_secondary, idx_primary;

    size_type nrow;
    size_type ncol;
    size_type nnz;			//number of non-zero values

    void check_dimensions(size_type expected, size_type actual) const {
        if (actual != expected) {
            throw std::runtime_error("Dimension mismatch");
        }
    }
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
					return v[i];
				}
			}
		} else {
			size_type upBound = col_idxs[col + 1];
			size_type lowBound = col_idxs[col];
			for(auto i = lowBound; i < upBound; ++i){
				if(row_idxs[i] == row){
					return v[i];
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
					return v[i];
				}
			}
		} else {
			size_type upBound = col_idxs[col + 1];
			size_type lowBound = col_idxs[col];
			for(auto i = lowBound; i < upBound; ++i){
				if(row_idxs[i] == row){
					return v[i];
				}
			}
		}
	}
}


template <typename T, Order O>
void Matrix<T, O>::insert_element(size_type row, size_type col, value_type x) {
    if (is_compressed()) {
		if constexpr (O == Order::ROW){
			size_type upBound = row_idxs[row + 1];
			size_type lowBound = row_idxs[row];
			for(auto i = lowBound; i < upBound; ++i){
				if(col_idxs[i] == col){
					v[i] = x;
				}
			}
		} else {
			size_type upBound = col_idxs[col + 1];
			size_type lowBound = col_idxs[col];
			for(auto i = lowBound; i < upBound; ++i){
				if(row_idxs[i] == row){
					v[i] = x;
				}
			}
		}
    } else {
        m_matrix[{row, col}] = x;
    }
}


template<typename T, Order O>
bool Matrix<T, O>::is_compressed() const {
    return v.empty() && col_idxs.empty() && row_idxs.empty() && !m_matrix.empty();
}


template <class T, Order O>
std::vector<T> operator*(Matrix<T, O> const &m, std::vector<T> const &v)
{
  std::vector<double> tmp;
  m.vecMultiply(v, tmp);
  return tmp;
}


template <class T, Order O>
void Matrix<T, O>::vecMultiply(const std::vector<T> &v, std::vector<T> &res) const {
    if (is_compressed()) {
        res.resize(nrow, T(0));
		for (size_type i = 0; i < nrow; i++) {
			for (size_type j = row_idxs[i]; j < row_idxs[i + 1]; j++) {
				res[i] += this->v[j] * v[col_idxs[j]];
			}
		}
    } else {
		if(v.size() != ncol)
			{
			std::cerr << " Vector must have the right size" << std::endl;
			std::exit(1);
			}
		res.resize(nrow, T(0));
		// for efficiency I use two different algorithms

		if constexpr(O == Order::ROW){
			// Classic A*v row by row
			for(const auto& [key, value]: m_matrix){
				res[key[1] / ncol] += value * v[key[0] % ncol];
			}
		} else {
			//result is a linear combination of the columns of A
			for(const auto& [key, value]: m_matrix){
				res[key[0] % nrow] += value * v[key[0] / nrow];
			}
		}
	}
}


// template <typename T>
// void Matrix<T, Order::ROW>::compress() {
//     if (is_compressed()) return;

// 	row_idxs.push_back(0);
// 	//compress sparse row
// 	for(const auto& [key, value]: m_matrix) {
// 		v.push_back(value);
// 		col_idxs.push_back(key[1]);
// 		if(row_idxs.back() != key[0]){
// 			row_idxs.push_back(v.size() - 1);
// 		}
// 	}

// 	m_matrix.clear();
// }

// template <typename T>
// void Matrix<T, Order::ROW>::uncompress() {
//     if (!m_matrix.empty()) return;

// 	for(unsigned int i = 0, r = 0; i < v.size(); ){
// 		if(i < row_idx[r + 1]){
// 			m_matrix[{r,col_idxs[i]}] = v[i];
// 			++i;
// 		} else {
// 			++r;
// 		}
// 	}
	
//     v.clear();
//     col_idxs.clear();
//     row_idxs.clear();
// }

// //valid from Col major uncompressed to compressed
// template <typename T>
// void Matrix<T, Order::COL>::compress() {
//     if (is_compressed()) return;

// 	col_idxs.push_back(0);
// 	//compress sparse row
// 	for(const auto& [key, value]: m_matrix) {
// 		v.push_back(value);
// 		row_idxs.push_back(key[1]);
// 		if(col_idxs.back() != key[0]){
// 			col_idxs.push_back(v.size() - 1);
// 		}
// 	}

// 	m_matrix.clear();
// }

// template <typename T>
// void Matrix<T, Order::COL>::uncompress() {
//     if (!m_matrix.empty()) return;

// 	for(unsigned int i = 0, c = 0; i < v.size(); ){
// 		if(i < col_idxs[r + 1]){
// 			m_matrix[{row_idxs[i],c}] = v[i];
// 			++i;
// 		} else {
// 			++c;
// 		}
// 	}
	
//     v.clear();
//     col_idxs.clear();
//     row_idxs.clear();
// }


template <class T, Order O>
void Matrix<T, O>::compress() {
	if (is_compressed()) return;

	size_type last_index = 0;
	// Reset or initialize index vectors
	idx_primary.push_back(0);

	for (const auto& [key, value] : m_matrix) {
		v.push_back(value);
		idx_secondary.push_back(key[O == Order::ROW ? 1 : 0]);
		if (last_index != key[O == Order::ROW ? 0 : 1]) {
			idx_primary.push_back(v.size() - 1);
			last_index = key[O == Order::ROW ? 0 : 1];
		}
	}
	idx_primary.push_back(v.size());
	m_matrix.clear();
}

template <class T, Order O>
void Matrix<T, O>::uncompress() {
	if (!m_matrix.empty()) return;

	for (size_type p = 0; p < idx_primary.size() - 1; ++p) {
		for (size_type i = idx_primary[p]; i < idx_primary[p + 1]; ++i) {
			key_type key = {O == Order::ROW ? p : idx_secondary[i], O == Order::ROW ? idx_secondary[i] : p};
			m_matrix[key] = v[i];
		}
	}

	v.clear();
	idx_secondary.clear();
	idx_primary.clear();
}


// }