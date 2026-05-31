#include "stdafx.h"
#include "graph.h"
void graph_t::initialize() { //对graph类进行初始化
#ifndef FINAL_RELEASE
	if (initialized)
		error("多次对graph_t类进行初始化。");
	initialized = true;
#endif
	init_constant(); //初始化需要用的常数
	this->strategy = STRATEGY_NORMAL; //设置默认的走棋策略
	std::memset(grades, 0, sizeof(grades));
	std::memset(line, 0, sizeof(line));
	//std::memset(closed, 0, sizeof(closed));
	//更新地图信息
	for (int r = 1; r <= GRAPH_WIDTH; ++r) {
		for (int c = 1; c <= GRAPH_WIDTH; ++c) {
			line[0][DIR_NUM0(r, c)][0] |= 1 | (1 << (GRAPH_WIDTH + 1));
			line[0][DIR_NUM0(r, c)][1] |= 1 | (1 << (GRAPH_WIDTH + 1));
			line[1][DIR_NUM1(r, c)][0] |= 1 | (1 << (GRAPH_WIDTH + 1));
			line[1][DIR_NUM1(r, c)][1] |= 1 | (1 << (GRAPH_WIDTH + 1));
			if (r == GRAPH_WIDTH || c == GRAPH_WIDTH) {
				int idx = INDEX[2][r][c];
				line[2][DIR_NUM2(r, c)][0] |= 1 | (1 << (idx + 1));
				line[2][DIR_NUM2(r, c)][1] |= 1 | (1 << (idx + 1));
			}
			if (r == GRAPH_WIDTH || c == 1) {
				int idx = INDEX[3][r][c];
				line[3][DIR_NUM3(r, c)][0] |= 1 | (1 << (idx + 1));
				line[3][DIR_NUM3(r, c)][1] |= 1 | (1 << (idx + 1));
			}
		}
	}
	//更新得分信息
	head.score_sum[0] = head.score_sum[1] = INF;
	head.score_max[0] = head.score_max[1] = INF;
	head.left_sum = head.right_sum = &head;
	head.left_max[0] = head.right_max[0] = &head;
	head.left_max[1] = head.right_max[1] = &head;
	for (int i = 0; i < SAVE_WIDTH; ++i) {
		for (int j = 0; j < SAVE_WIDTH; ++j) {
			nodes[i][j].location = pos(i, j);
			if (i == 0 || i == GRAPH_WIDTH + 1 || j == 0 || j == GRAPH_WIDTH + 1) {
				nodes[i][j].type = WALL;
			}
			else {
				nodes[i][j].type = SPACE;
			}
		}
	}
	for (int i = 1; i <= GRAPH_WIDTH; ++i) {
		for (int j = 1; j <= GRAPH_WIDTH; ++j) {
			enter(nodes[i][j]);
		}
	}
}
void graph_t::print(pos mark) const { //输出地图并标出mark对应的位置,便于提示按钮调用
	char buffer[SAVE_WIDTH][SAVE_WIDTH + 1];
	for (int i = 0; i < SAVE_WIDTH; ++i) {
		for (int j = 0; j < SAVE_WIDTH; ++j) {
			buffer[i][j] = nodes[i][j].type;
		}
		buffer[i][SAVE_WIDTH] = '\n';
	}
	if (mark != pos(-1, -1)) {
#ifndef FINAL_RELEASE
		if (!mark.valid())//错误处理--位置在地图外
			error("调用graph::print的mark参数引用了一个错误的位置。");
#endif
		char& place = buffer[mark.row][mark.column];
		if (std::islower(place))
			place = std::toupper(place);
		else
			place = STAR;
	}
	std::fwrite(buffer, sizeof(buffer), 1, stdout);
}
//在棋盘的position位置放置棋子，类型为id
void graph_t::set(pos position, int id, bool adjust) {
#ifndef FINAL_RELEASE //如果不是最终版本，则对参数进行测试
	if (!this->initialized)
		error("试图对未初始化的graph_t类调用set成员函数。");
	if (id != 0 && id != 1)
		error("调用graph_t::set的id参数错误。");
	if (!position.valid())
		error("调用graph_t::set的position参数越界。");
	if (nodes[position.row][position.column].type != SPACE) {
		print(position);
		error("试图覆盖棋盘中的棋子。");
	}
#endif
	order.push(position);
	char type = CHARACTER[id];
	node& base = nodes[position.row][position.column];
	int r = base.location.row, c = base.location.column;
	base.type = type;
	line[0][DIR_NUM0(r, c)][id] |= 1 << INDEX[0][r][c];
	line[1][DIR_NUM1(r, c)][id] |= 1 << INDEX[1][r][c];
	line[2][DIR_NUM2(r, c)][id] |= 1 << INDEX[2][r][c];
	line[3][DIR_NUM3(r, c)][id] |= 1 << INDEX[3][r][c];
	base.extract_sum();
	for (int i = 0; i < 2; ++i) {
		grades[i] -= DUMP_SCORE(base.score_sum[i]);
		if (base.score_max[i] > SCORE_MAX_BOUND)
			base.extract_max(i);
	}
	for (auto direct : around8) { //枚举8个方向
		int number = 0, expand = 0;
		bool pass_me = false, pass_other = false;
		pos now = position + direct;
		for (int num_spaces = 0; num_spaces < 3 + expand; now += direct) {
			node& nd = nodes[now.row][now.column];
			if (++number == 2 && num_spaces == 1 && nd.type == CHARACTER[id ^ 1]) // _W
				expand = 1;
			if (nd.type == SPACE) {
				num_spaces += 1;
				base.dump(nd, CONVERT_POS(direct));
				if (pass_me)
					maintain_single(nd, direct, id, adjust);
				else if (pass_other)
					maintain_single(nd, direct, id ^ 1, adjust);
				else
					maintain(nd, direct, adjust);
			}
			else if (nd.type == CHARACTER[id])
				pass_me = true;
			else if (nd.type == CHARACTER[id ^ 1])
				pass_other = true;
			else //遇到墙
				break;
			if (pass_me && pass_other)
				break;
		}
	}
}
void graph_t::undo() { //撤销上一次下的棋
#ifndef FINAL_RELEASE
	if (order.empty())//特判空棋局
		error("试图对空图调用undo操作。");
#endif
	pos position = order.top();//记录上一步棋，即要撤销的棋
	order.pop();//将要插销的棋弹栈
	node& base = nodes[position.row][position.column];
	int r = base.location.row, c = base.location.column;
	int id = base.type == MACHINE ? MACHINE_ID : PERSON_ID;
	//更新地图信息与得分信息
	line[0][DIR_NUM0(r, c)][id] &= ~(1 << INDEX[0][r][c]);
	line[1][DIR_NUM1(r, c)][id] &= ~(1 << INDEX[1][r][c]);
	line[2][DIR_NUM2(r, c)][id] &= ~(1 << INDEX[2][r][c]);
	line[3][DIR_NUM3(r, c)][id] &= ~(1 << INDEX[3][r][c]);
	base.type = SPACE;
	resume(base);
	base.resume_sum();
	for (int i = 0; i < 2; ++i) {
		grades[i] += DUMP_SCORE(base.score_sum[i]);
		if (base.score_max[i] > SCORE_MAX_BOUND)
			base.resume_max(i);
	}
}
void graph_t::debug(int seed) { //通过多次随机的走棋和撤销操作，来测试graph_t类是否正确运行，仅供测试使用，不可在实际运行时使用
	std::srand(seed);
	if (!order.empty())
		error("debug前棋盘必须为空。");
	using graph_state_t = stack<debug_t, SAVE_WIDTH* SAVE_WIDTH + 5>;
	graph_state_t& graph_state = *new graph_state_t();
	std::clock_t start_time = std::clock(); //Debug计时器启动
	int times = 0, nonspaces = 0;
	while (times < 1000000) { //动态维护得分/棋局状态功能测试
		int type = std::rand() % 2;
		int cnt = std::rand() % 10;
		if (type) {
			for (int j = 0; j < cnt; ++j) {
				pos position = pos(std::rand() % GRAPH_WIDTH + 1, std::rand() % GRAPH_WIDTH + 1);
				if (get(position) == SPACE) {
					++nonspaces;
					++times;
					int id = std::rand() % 2;
					if (score(position, CHARACTER[id]) != nodes[position.row][position.column].score_sum[id]) {
						this->print(position);
						error("graph动态维护得分错误。");
					}
					set(position, id);
				}
			}
		}
		else {
			for (int j = 0; j < cnt; ++j) {
				if (nonspaces <= 0)
					break;
				--nonspaces;
				undo();
			}
		}
	}
	times = 0, nonspaces = 0;
	while (times < 1000000) { //判断弹栈的棋是否与上一步棋相同
		int type = std::rand() % 2;
		int cnt = std::rand() % 10;
		if (type) {
			for (int j = 0; j < cnt; ++j) {
				pos position = pos(std::rand() % GRAPH_WIDTH + 1, std::rand() % GRAPH_WIDTH + 1);
				if (get(position) == SPACE) {
					++nonspaces;
					++times;
					int id = std::rand() % 2;
					graph_state.push(debug_t(this));
					set(position, id);
				}
			}
		}
		else {
			for (int j = 0; j < cnt; ++j) {
				if (nonspaces <= 0)
					break;
				--nonspaces;
				undo();
				if (!graph_state.top().check())
					error("graph中有bug。");
				graph_state.pop();
			}
		}
	}
	times = 0, nonspaces = 0;
	while (times < 1000000) { //动态维护棋局状态功能测试
		int type = std::rand() % 2;
		int cnt = std::rand() % 10;
		if (type) {
			for (int j = 0; j < cnt; ++j) {
				pos position = pos(std::rand() % GRAPH_WIDTH + 1, std::rand() % GRAPH_WIDTH + 1);
				if (get(position) == SPACE) {
					++nonspaces;
					++times;
					int id = std::rand() % 2;
					set(position, id);
				}
			}
		}
		else {
			for (int j = 0; j < cnt; ++j) {
				if (nonspaces <= 0)
					break;
				--nonspaces;
				undo();
			}
		}
	}
	std::clock_t end_time = std::clock();
	printf("Debug用时%f秒。\n", static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC);
	clear();
}
//计算当前棋局在位置position且方向为direction、类型为id的得分
inline int graph_t::score(pos position, int direction, int id) {
#ifndef FINAL_RELEASE
	if (!position.valid())//该方向越界或已经有棋子在这里或position参数不在约定范围
		error("graph_t::score的position参数错误。");
#endif
	int r = position.row, c = position.column;
	int st = line[direction][DIR_NUM[direction][r][c]][id];
	int st2 = line[direction][DIR_NUM[direction][r][c]][id ^ 1];
	int idx = INDEX[direction][r][c];
	int rg = STATE_RANGE[st2][idx];
	int len = RANGE_LENGTH[rg];
	int beg = STATE_START[st2][idx];
	int extract = (st & rg) >> beg;
	int loc = idx - beg;
	return SCORE_TABLE[len][extract][loc];
}
//对结点self的direct方向进行维护
inline void graph_t::maintain_single(node& self, pos direct, int i, bool adjust) {
#ifndef FINAL_RELEASE
	if (direct == pos(0, 0))//偏移量不能为0，保证新位置与原位置不同，避免死循环
		error("maintain的direct参数不能是零向量。");
#endif
	int vector = CONVERT_POS(direct);
	this->grades[i] -= DUMP_SCORE(self.score_sum[i]);
	self.score_sum[i] -= self.direct_score[i][vector];
	self.score_sum[i] -= ADDITION4[DR(0)][DR(1)][DR(2)][DR(3)];
	self.direct_score[i][vector] = this->score(self.location, vector, i);
#ifndef FINAL_RELEASE
	for (int j = 0; j < 4; ++j) {
		if (self.direct_score[i][j] >= SCORE_UPPER_BOUND || DR(j) == -1) {//方向得分错误，具体原因需进一步分析
			error("错误的方向得分：" + std::to_string(self.direct_score[i][j]));
		}
	}
#endif
	self.score_sum[i] += self.direct_score[i][vector];
	self.score_sum[i] += ADDITION4[DR(0)][DR(1)][DR(2)][DR(3)];
	this->grades[i] += DUMP_SCORE(self.score_sum[i]);
	int max_sc = -INF, max_sc2 = -INF;
	for (int j = 0; j < 4; ++j) {
		if (self.direct_score[i][j] > max_sc)
			max_sc2 = max_sc, max_sc = self.direct_score[i][j];
		else if (self.direct_score[i][j] > max_sc2)
			max_sc2 = self.direct_score[i][j];
	}
	update_max(self, i, max_sc + max_sc2 + ADDITION2[RANK[max_sc]][RANK[max_sc2]]);
	if (adjust) {
		adjust_sum(self);
	}
}
//对结点self的direct方向进行维护
inline void graph_t::maintain(node& self, pos direct, bool adjust) {
#ifndef FINAL_RELEASE //最终版本，参数测试已经完成
	if (direct == pos(0, 0))//偏移量不能为0，保证新位置与原位置不同，避免死循环
		error("maintain的direct参数不能是零向量。");
#endif
	for (int i = 0; i < 2; ++i) {
		int vector = CONVERT_POS(direct);
		this->grades[i] -= DUMP_SCORE(self.score_sum[i]);
		self.score_sum[i] -= self.direct_score[i][vector];
		self.score_sum[i] -= ADDITION4[DR(0)][DR(1)][DR(2)][DR(3)];
		self.direct_score[i][vector] = this->score(self.location, vector, i);
#ifndef FINAL_RELEASE
		for (int j = 0; j < 4; ++j) {
			if (self.direct_score[i][j] >= SCORE_UPPER_BOUND || DR(j) == -1) {//方向得分错误，具体原因需进一步分析
				error("错误的方向得分：" + std::to_string(self.direct_score[i][j]));
			}
		}
#endif
		self.score_sum[i] += self.direct_score[i][vector];
		self.score_sum[i] += ADDITION4[DR(0)][DR(1)][DR(2)][DR(3)];
		this->grades[i] += DUMP_SCORE(self.score_sum[i]);
		int max_sc = -INF, max_sc2 = -INF;
		for (int j = 0; j < 4; ++j) {
			if (self.direct_score[i][j] > max_sc)
				max_sc2 = max_sc, max_sc = self.direct_score[i][j];
			else if (self.direct_score[i][j] > max_sc2)
				max_sc2 = self.direct_score[i][j];
		}
		update_max(self, i, max_sc + max_sc2 + ADDITION2[RANK[max_sc]][RANK[max_sc2]]);
	}
	if (adjust) {
		adjust_sum(self);
	}
}
inline void graph_t::enter(node& self) { //据一个节点信息，更新当期棋盘状态
	self.score_sum[0] = self.score_sum[1] = 0;
	self.insert_sum_after(this->head);
	for (int i = 0; i < 4; ++i) {
		maintain(self, reverse[i]);
	}
}
inline void graph_t::adjust_sum(node& self) { //优化，参数为优化所在的节点
	node* L = self.left_sum, * R = self.right_sum;
	if (self.score_sum[0] + self.score_sum[1] > L->score_sum[0] + L->score_sum[1]) {
		while (L != &this->head && self.score_sum[0] + self.score_sum[1] > L->score_sum[0] + L->score_sum[1]) {
			L = L->left_sum; //只需要判断其左节点信息，将其它子节点信息与本节点部分无需调用原理处理过程的信息减除
		}
		self.extract_sum();
		self.insert_sum_after(*L);
	}
	else if (R != &this->head && self.score_sum[0] + self.score_sum[1] < R->score_sum[0] + R->score_sum[1]) {
		while (R != &this->head && self.score_sum[0] + self.score_sum[1] < R->score_sum[0] + R->score_sum[1]) {
			R = R->right_sum; //只需要判断其右节点信息，将其它子节点信息与本节点部分无需调用原理处理过程的信息减除
		}
		self.extract_sum();
		self.insert_sum_after(*R->left_sum);
	}
}
inline void graph_t::adjust_max(node& self, int id, node* L, node* R) { //优化，参数为优化所在的节点和子节点
	if (self.score_max[id] > L->score_max[id]) {
		while (L != &this->head && self.score_max[id] > L->score_max[id]) {
			L = L->left_max[id]; //只需要判断其左节点信息，将其它子节点信息与本节点部分无需调用原理处理过程的信息减除
		}
		self.insert_max_after(*L, id);
	}
	else {
		while (R != &this->head && self.score_max[id] < R->score_max[id]) {
			R = R->right_max[id]; //只需要判断其右节点信息，将其它子节点信息与本节点部分无需调用原理处理过程的信息减除
		}
		self.insert_max_after(*R->left_max[id], id);
	}
}
inline void graph_t::update_max(node& self, int id, int new_score) {//更新信息
	int last_score = self.score_max[id];
	self.score_max[id] = new_score;
	if (last_score <= SCORE_MAX_BOUND) {
		if (new_score > SCORE_MAX_BOUND) {
			adjust_max(self, id, this->head.left_max[id], &this->head);
		}
	}
	else if (last_score > SCORE_MAX_BOUND) {
		if (new_score <= SCORE_MAX_BOUND) { //优化
			self.extract_max(id);
		}
		else {
			self.extract_max(id);
			adjust_max(self, id, self.left_max[id], self.right_max[id]);
		}
	}
}
inline void graph_t::back(saver& self) {
	node* owner = self.owner;
	if (owner->score_max[0] > SCORE_MAX_BOUND)
		owner->extract_max(0);
	if (owner->score_max[1] > SCORE_MAX_BOUND)
		owner->extract_max(1);
	owner->extract_sum();
	owner->direct_score[0][self.direct] = self.score_last[0];
	owner->direct_score[1][self.direct] = self.score_last[1];
	owner->score_max[0] = self.score_max[0];
	owner->score_max[1] = self.score_max[1];
	this->grades[0] -= DUMP_SCORE(owner->score_sum[0]);
	this->grades[1] -= DUMP_SCORE(owner->score_sum[1]);
	owner->score_sum[0] = self.score_sum[0];
	owner->score_sum[1] = self.score_sum[1];
	this->grades[0] += DUMP_SCORE(owner->score_sum[0]);
	this->grades[1] += DUMP_SCORE(owner->score_sum[1]);
	if (owner->score_max[0] > SCORE_MAX_BOUND)
		owner->insert_max_after(*self.prior_max[0], 0);
	if (owner->score_max[1] > SCORE_MAX_BOUND)
		owner->insert_max_after(*self.prior_max[1], 1);
	owner->insert_sum_after(*self.prior_sum);
}
inline void node::insert_sum_after(node& nd) {//在某节点后插入sum
	this->right_sum = nd.right_sum;
	this->left_sum = &nd;
	this->right_sum->left_sum = this;
	this->left_sum->right_sum = this;
}
inline void node::extract_sum() {//α/β剪枝时确定其限制值
	this->left_sum->right_sum = this->right_sum;
	this->right_sum->left_sum = this->left_sum;
}
inline void node::resume_sum() {//返回搜索值
	this->left_sum->right_sum = this;
	this->right_sum->left_sum = this;
}
inline void node::insert_max_after(node& nd, int id) {//在某节点后插入sum
	this->right_max[id] = nd.right_max[id];
	this->left_max[id] = &nd;
	this->right_max[id]->left_max[id] = this;
	this->left_max[id]->right_max[id] = this;
}
inline void node::extract_max(int id) {//α/β剪枝时确定其限制值
	this->left_max[id]->right_max[id] = this->right_max[id];
	this->right_max[id]->left_max[id] = this->left_max[id];
}
inline void node::resume_max(int id) {//返回搜索值
	this->left_max[id]->right_max[id] = this;
	this->right_max[id]->left_max[id] = this;
}
inline saver::saver(node& nd, int dir) {//保存某节点信息
	this->owner = &nd;
	this->direct = dir;
	this->score_last[0] = nd.direct_score[0][dir];
	this->score_last[1] = nd.direct_score[1][dir];
	this->score_max[0] = nd.score_max[0];
	this->score_max[1] = nd.score_max[1];
	this->score_sum[0] = nd.score_sum[0];
	this->score_sum[1] = nd.score_sum[1];
	this->prior_max[0] = nd.left_max[0];
	this->prior_max[1] = nd.left_max[1];
	this->prior_sum = nd.left_sum;
}
void graph_set(graph_t& map, int id, std::initializer_list<pos> list) {
	for (auto position : list)
		map.set(position, id);
}