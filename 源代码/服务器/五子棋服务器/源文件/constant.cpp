#include "stdafx.h"
unsigned int STATE_RANGE[STATE_ARRAY_SIZE][SAVE_WIDTH]; //状态范围
int STATE_START[STATE_ARRAY_SIZE][SAVE_WIDTH]; //起始状态
int RANGE_LENGTH[STATE_ARRAY_SIZE]; //范围的长度
int STATE_LENGTH[STATE_ARRAY_SIZE]; //状态的长度
int BIT_REVERSE[STATE_ARRAY_SIZE]; //按位反转
int CONVERT_BASE[3][3] = { { 2, 1, 3 }, { 0, -1, 0 }, { 3, 1, 2 } }; //转换基
int RANK[SCORE_UPPER_BOUND]; //得分的等级
int ADDITION4[NUM_RANKS][NUM_RANKS][NUM_RANKS][NUM_RANKS]; //四个方向得分组合后的得分
int ADDITION2[NUM_RANKS][NUM_RANKS] = {  //两个方向得分组合后的得分
	{0, 0, 0, 0, 0, 0, 0},
	{0, 1000, 2500, 3500, 3500, 0, 0},
	{0, 2500, 3500, 4500, 4500, 0, 0},
	{0, 3500, 4500, 14000, 20000, 0, 0},
	{0, 3500, 4500, 20000, 30000, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
};
int NUMSPACES[STATE_ARRAY_SIZE]; //空格的数量
int NUMTHINGS[STATE_ARRAY_SIZE]; //非空白的数量
int SCORE_TABLE[GRAPH_WIDTH/*状态的长度-1*/][1 << GRAPH_WIDTH/*状态*/][GRAPH_WIDTH/*要估分的位置-1*/];
int INDEX[4/*行 列 主 副*/][SAVE_WIDTH][SAVE_WIDTH];
int DIR_NUM[4/*行 列 主 副*/][SAVE_WIDTH][SAVE_WIDTH];
char map[SAVE_WIDTH];
int WEIGHT[MAX_STATE][MAX_WALL]; //状态的权重
//获取从position沿着direct方向走遇到的墙的类型
wall_type get_wall_type(int position, int direct) {
	position += direct;
	char now = map[position];
	if (now == PERSON)
		return wall_type::empty;
	else if (now != SPACE)
		return wall_type::just_wall;
	int next_pos = position + direct;
	char next = map[next_pos];
	if (next == SPACE || next == PERSON)
		return wall_type::empty;
	else
		return wall_type::space_wall;
}
//计算位置position的得分
int score(int position) {
	int result = 0;//结果记录在result中,初始化result = 0
	int L, R;
	unsigned int state = 0;
	char last = map[position];
	map[position] = PERSON;
	// NUMSPACES[state]储存对应状态信息，用state作为下标索引，用数组模拟树
	for (L = position; ; L += 1) { // 建树
		char ch = map[L];
		if (ch == PERSON)
			state |= 1;
		if (ch != PERSON && ch != SPACE)
			break;
		if (NUMSPACES[state] > 2 || NUMTHINGS[state] > 5)
			break;
		state <<= 1;
	}
	L -= 1;
	while (map[L] != PERSON)
		L -= 1;
	unsigned int last_state = state = 0;
	R = L;
	for (int last_pos = -1;;) {
		bool update = false; //是否需要更新信息
		for (;;) {
			char ch = map[R];
			if (ch == PERSON && R != last_pos)
				update = true; //需要更新信息
			if (ch == PERSON)
				state |= 1, last_pos = R, last_state = state;
			if (ch != SPACE && ch != PERSON)
				break;
			if (NUMSPACES[state] > 2 || NUMTHINGS[state] >= 5)
				break;
			state <<= 1; //state <<= 1, 变为子状态下标
			R -= 1;
		}
		if (update) { //如果需要更新信息，对墙壁类型，递归状态，result进行更新
			R = last_pos, state = last_state;
			wall_type left_wall_type = get_wall_type(L, 1);
			wall_type right_wall_type = get_wall_type(R, -1);
			//在result计算时，默认双方都很聪明，可以选择对自己最有利的解，故当前状态的result由子节点中result最大的决定。
			result = std::max(result, WEIGHT[state][JOIN_WALL_TYPE(left_wall_type, right_wall_type)]);
			if (right_wall_type != wall_type::empty)
				break;
		}
		if (L == position)
			break;
		L -= 1;
		while (map[L] != PERSON)
			L -= 1;
		state ^= 1 << STATE_LENGTH[state];
	}
	map[position] = last;
	return result;
}
//初始化常量
void init_constant() {
	static bool have_called = false; //静态变量，第一次赋值时生效
	if (!have_called) //保证 have_called 为true
		have_called = true;
	else
		return;
#ifndef FINAL_RELEASE
	std::memset(RANK, INVALID, sizeof(RANK));
#endif
	FILE* fp = fopen("weight.db", "rb");
	fread(WEIGHT, sizeof(WEIGHT), 1, fp);
	fclose(fp);
	/*  难度等级在ui界面中使用可连续拖动的滑轮调节，滑轮根据所在位置返回一个值
		越靠左表示难度越低，对应的返回值也就越小，这里初始化rank与返回值的关系
		返回值范围是0-110000，对应如下7个等级。
		后台实际计算也是按ui界面实际返回值对应的rank等级来确定
	*/
	for (int i = 0; i <= 1000; ++i)
		RANK[i] = 0;
	for (int i = 1001; i <= 2000; ++i)
		RANK[i] = 1;
	for (int i = 2001; i <= 4000; ++i)
		RANK[i] = 2;
	for (int i = 5000; i <= 8500; ++i)
		RANK[i] = 3;
	for (int i = 9000; i <= 14000; ++i)
		RANK[i] = 4;
	for (int i = 45000; i <= 49000; ++i)
		RANK[i] = 5;
	for (int i = 100000; i <= 110000; ++i)
		RANK[i] = 6;
	//初始化每个等级在运行时需要的信息
	for (int i = 0; i < NUM_RANKS; ++i) {
		for (int j = 0; j < NUM_RANKS; ++j) {
			for (int k = 0; k < NUM_RANKS; ++k) {
				for (int l = 0; l < NUM_RANKS; ++l) {
					int arr[4] = { i, j, k, l };
					std::sort(std::begin(arr), std::end(arr), std::greater<int>());
					int leader = ADDITION2[arr[0]][arr[1]];
					ADDITION4[i][j][k][l] = leader +
						std::min(leader, ADDITION2[4][arr[2]]) +
						std::min(leader, ADDITION2[4][arr[3]]);
				}
			}
		}
	}
	for (int j = 0; j < STATE_ARRAY_SIZE; ++j) {
		int i = 31;
		for (; i >= 0; --i)
			if (j & (1 << i))
				break;
		STATE_LENGTH[j] = i;
		for (int k = 0; k <= i; ++k) {
			if (j & (1 << k)) {
				NUMTHINGS[j] += 1;
				BIT_REVERSE[j] |= 1 << (i - k);
			}
		}
		NUMSPACES[j] = i - NUMTHINGS[j] + 1;
	}
	RANGE_LENGTH[0] = -1;
	for (int j = 1; j < STATE_ARRAY_SIZE; ++j) {
		int b = 0, e = SAVE_WIDTH;
		for (; b < SAVE_WIDTH; ++b)
			if (j & (1 << b))
				break;
		for (; e >= 0; --e)
			if (j & (1 << e))
				break;
		RANGE_LENGTH[j] = e - b;
	}
	std::memset(STATE_RANGE, 0, sizeof(STATE_RANGE));
	for (int j = 0; j < STATE_ARRAY_SIZE; ++j) {
		for (int p = 0; p < SAVE_WIDTH; ++p) {
			if (j & (1 << p)) {
				STATE_START[j][p] = INVALID;
				STATE_RANGE[j][p] = INVALID;
				continue;
			}
			int b = p, e = p;
			for (; b >= 0; --b)
				if (j & (1 << b))
					break;
			STATE_START[j][p] = ++b;
			for (; e < SAVE_WIDTH; ++e)
				if (j & (1 << e))
					break;
			for (int i = b; i < e; ++i)
				STATE_RANGE[j][p] |= 1 << i;
		}
	}
#ifndef FINAL_RELEASE
	std::memset(SCORE_TABLE, INVALID, sizeof(SCORE_TABLE));
#endif
	for (int i = 1; i <= GRAPH_WIDTH; ++i) {
		for (int j = 0; j < (1 << i); ++j) {
			map[0] = map[i + 1] = WALL;
			for (int k = 0; k < i; ++k) {
				if (j & (1 << k))
					map[k + 1] = PERSON;
				else
					map[k + 1] = SPACE;
			}
			for (int p = 0; p < i; ++p) {
				int result = j & (1 << p) ? INVALID : score(p + 1);
				SCORE_TABLE[i - 1][j][p] = result;
			}
		}
	}
	for (int r = 1; r <= GRAPH_WIDTH; ++r) {
		for (int c = 1; c <= GRAPH_WIDTH; ++c) {
			INDEX[0][r][c] = c;
			INDEX[1][r][c] = r;
			INDEX[2][r][c] = std::min(r, c);
			INDEX[3][r][c] = std::min(r, GRAPH_WIDTH - c + 1);
		}
	}
	for (int r = 1; r <= GRAPH_WIDTH; ++r) {
		for (int c = 1; c <= GRAPH_WIDTH; ++c) {
			DIR_NUM[0][r][c] = DIR_NUM0(r, c);
			DIR_NUM[1][r][c] = DIR_NUM1(r, c);
			DIR_NUM[2][r][c] = DIR_NUM2(r, c);
			DIR_NUM[3][r][c] = DIR_NUM3(r, c);
		}
	}
}