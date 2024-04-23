namespace algebra 
{


//use enable if
template<typename StorageOrder>
bool operator< (const std::array<std::size_t,int>& lhs, 
	const std::array<std::size_t,2>& rhs) const
{
	return lhs < rhs;
}

enum StorageOrder {row_wise, col_wise};

template <typename VariableType, typename StorageOrder> 
class Matrix
{
	public:
		Matrix () = default; 
		Matrix (unsigned int n_rows, unsigned int n_cols);

		void resize(unsigned int n_rows, unsigned int n_cols);l

		void add_element(int row, int col, VariableType x)
		void compress();
		void uncompress();

		const bool is_compressed();

		VariableType operator()(unsigned int i, unsigned int j);
		VariableType const operator()(unsigned int i, unsigned int j);

		friend operator*(Matrix<VariableType, StorageOrder> matrix, 
			std::vector<VariableType> vector);

		friend operator*(Matrix<VariableType, StorageOrder> matrix1, 
			Matrix<VariableType, StorageOrder> matrix2);

		//placeholder, decide what the method should do; 
		void matrix_market_reader();

	private:
		//map to handle the matrix in an uncompressed format
		std::map<std::array<std::size_t,2>,VariableType> m_matrix;

		//three vectors required to store the matrix in compressed sparse row/column
		std::vector<VariableType> v;       
		std::vector<unsigned int> col_idxs;    
		std::vector<unsigned int> row_idxs; 
};





}