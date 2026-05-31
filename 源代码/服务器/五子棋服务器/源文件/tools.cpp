#include "stdafx.h"
void error(int reason) { //显示错误信息reason
	MessageBoxA(0, std::to_string(reason).c_str(), "Error", 0);
	exit(EXIT_FAILURE);
}
void error(const std::string reason) { //显示错误信息reason后抛出异常
	//throw reason;
	MessageBoxA(0, reason.c_str(), "Error", 0);
	throw reason;
	exit(EXIT_FAILURE);
}
std::ostream &operator<< (std::ostream &os, pos location) { //输出点location的坐标
	os << '(' << location.row << ", " << location.column << ')';
	return os;
}
void print(unsigned int state, int left_wall, int right_wall) { //输出状态state
	static const char *left_wall_str[] = { "", "W_", "W" };
	static const char *right_wall_str[] = { "", "_W", "W" };
	std::string str(left_wall_str[left_wall]);
	for (int i = STATE_LENGTH[state]; i >= 0; --i) {
		if (state & (1 << i))
			str.push_back('O');
		else
			str.push_back('_');
	}
	str += right_wall_str[right_wall];
	print(str);
}
void print_bits(unsigned int state) { //将状态state以二进制的形式输出
	std::bitset<SAVE_WIDTH> bits(state);
	print(bits);
}