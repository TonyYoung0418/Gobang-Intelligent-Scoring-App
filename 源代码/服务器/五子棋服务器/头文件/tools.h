#ifndef TOOLS_GUARD //文件保护符
#define TOOLS_GUARD
//在控制台下用print输出信息，图形界面下用warn输出信息
template<typename T> void print(const T& sth) {
	std::cout << sth << std::endl;
}
template<typename T> void warn(const T& sth) {
	std::stringstream ss;
	ss << sth;
	MessageBoxA(0, ss.str().c_str(), "Warning", 0);
}
void print_bits(unsigned int state); //按位输出int类型表示的状态state
void print(unsigned int state, int left_wall, int right_wall);
void error(int reason);
void error(const std::string reason);
//坐标（向量）类，与std::pair<int, int>类似，但是实现了向量加法、减法、数成向量等操作。
class pos {
public:
	int row;
	int column;
public:
	pos() = default;
	pos(int r, int c) : row(r), column(c) {}
	inline pos operator+ (const pos& rhs) const { //重载 +
		return pos(this->row + rhs.row, this->column + rhs.column);
	}
	inline pos operator* (const pos& rhs) const { //重载 *
		return pos(this->row * rhs.row, this->column * rhs.column);
	}
	inline pos operator* (int num) const { //重载 *
		return pos(this->row * num, this->column * num);
	}
	inline pos operator/ (const pos& rhs) const { //重载 /
		return pos(this->row / rhs.row, this->column / rhs.column);
	}
	inline void operator+= (const pos& rhs) { //重载 +=
		this->row += rhs.row;
		this->column += rhs.column;
	}
	inline void operator-= (const pos& rhs) { //重载 -=
		this->row -= rhs.row;
		this->column -= rhs.column;
	}
	inline void operator/= (const pos& rhs) { //重载 /=
		this->row /= rhs.row;
		this->column /= rhs.column;
	}
	inline pos operator- (const pos& rhs) const { //重载 -
		return pos(this->row - rhs.row, this->column - rhs.column);
	}
	inline pos operator- () const { //重载 -
		return pos(-this->row, -this->column);
	}
	inline bool operator== (const pos& rhs) const { //重载 ==
		return this->row == rhs.row && this->column == rhs.column;
	}
	inline bool operator!= (const pos& rhs) const { //重载 !=
		return this->row != rhs.row || this->column != rhs.column;
	}
	inline bool operator< (const pos& rhs) const { //重载 <
		return this->row < rhs.row || (this->row == rhs.row && this->column < rhs.column);
	}
	inline bool valid() const { //判断当前位置是否合法（没有超出棋盘边界）
		return row > 0 && column > 0 && row <= GRAPH_WIDTH && column <= GRAPH_WIDTH;
	}
	inline int length() const {
		return std::abs(this->row) + std::abs(this->column);
	}
};
std::ostream& operator<< (std::ostream& os, pos location);
//与一个点相近的16点的相对位置
const pos around16[16] = {
	pos(-1, -1), pos(1, 1), pos(-1, 1), pos(1, -1), pos(-1, 0), pos(0, -1), pos(0, 1), pos(1, 0),
	pos(-2, -2), pos(-2, 0), pos(-2, 2), pos(0, -2), pos(0, 2), pos(2, -2), pos(2, 0), pos(2, 2)
};
//一个点的相邻的8个点的相对位置
const pos around8[8] = {
	pos(-1, -1), pos(-1, 0), pos(-1, 1), pos(0, -1), pos(0, 1), pos(1, -1), pos(1, 0), pos(1, 1)
};
//上下左右
const pos around4[4] = {
	pos(0, 1), pos(1, 0), pos(1, 1), pos(1, -1)
};
const pos reverse[4] = { pos(-1, 0), pos(-1, -1), pos(0, -1), pos(1, -1) };
class range_iter { //范围迭代器类型
private:
	const pos* now;
	pos base;
public:
	range_iter(const pos* n, pos b) : now(n), base(b) {}
	pos operator* () const {
		return *now + base;
	}
	void operator++() {
		++now;
	}
	bool operator!=(const range_iter& rhs) const {
		return this->now != rhs.now;
	}
};
class range {
private:
	pos base;
	int size;
public:
	range(pos b, int sz = 16) : base(b), size(sz) {}
	range(int r, int c, int sz = 16) : base(r, c), size(sz) {}
	range_iter begin() const {
		return range_iter(around16, base);
	}
	range_iter end() const {
		return range_iter(around16 + size, base);
	}
};
#endif
