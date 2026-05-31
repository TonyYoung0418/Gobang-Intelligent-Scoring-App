#ifndef GRAPH_GUARD
#define GRAPH_GUARD
class graph_t;
class node_iter;
class node;
class debug_t;
class kill_iter;
class saver {
public:
	node *owner; //指向拥有者的指针
	int direct; //当前的方向
	int score_last[2]; //上一次位置的得分
	int score_max[2]; //历史最大得分
	int score_sum[2]; //得分和
	node *prior_max[2]; //最大得分结点的指针
	node *prior_sum; //得分和的结点指针
public:
	inline saver() = default;
	inline saver(node &nd, int dir);
};
class base_node {
	friend class debug_t;
protected:
	int direct_score[2][4]; //各个方向的得分
	int score_sum[2]; //得分和
	int score_max[2]; //历史最大得分
protected:
	base_node() : direct_score(), score_sum(), score_max() {}
	//比较两个结点是否相等（对所有的成员变量就行比较）
	inline bool operator== (const base_node &rhs) const {
		if (std::memcmp(this->direct_score, rhs.direct_score, sizeof(this->direct_score)) != 0)
			return false;
		if (std::memcmp(this->score_sum, rhs.score_sum, sizeof(this->score_sum)) != 0)
			return false;
		if (std::memcmp(this->score_max, rhs.score_max, sizeof(this->score_max)) != 0)
			return false;
		return true;
	}
	//比较两个结点是否不等
	inline bool operator!= (const base_node &rhs) const {
		return !(*this == rhs);
	}
};
class node : public base_node {
	friend class graph_t;
	friend class node_iter;
	friend class const_node_iter;
	friend class saver;
	friend class kill_iter;
private:
	pos location; //当前结点对应的位置（坐标）
	char type; //当前位置摆放的棋子的类型
	node *left_sum; //左侧的结点（组成一个链表）
	node *right_sum; //右侧的结点（组成一个链表）
	node *left_max[2]; 
	node *right_max[2];
	stack<saver, 8 * 4 + 1> save; //状态栈（由于恢复到之前的状态）
	unsigned int mark;
private:
	node() : location(pos(-1, -1)), type(NONE), left_sum(), right_sum(),
		left_max(), right_max(), mark(0){}
	inline void insert_sum_after(node &nd);
	inline void extract_sum();
	inline void resume_sum();
	inline void insert_max_after(node &nd, int id);
	inline void extract_max(int id);
	inline void resume_max(int id);
	inline void dump(node &nd, int direct) {
		save.expand() = saver(nd, direct);
	}
public:
	inline pos where() const { //返回当前结点对应的位置
		return location;
	}
	inline int score() const { //返回当前位置的得分
		return score_sum[0] + score_sum[1];
	}
};
class node_iter { //结点的迭代器类型
private:
	node *ptr;
public:
	node_iter(node *data) : ptr(data){}
	bool operator== (node_iter rhs) const {
		return ptr == rhs.ptr;
	}
	bool operator!= (node_iter rhs) const {
		return ptr != rhs.ptr;
	}
	void operator++ () {
		ptr = ptr->right_sum;
	}
	node *operator-> () {
		return ptr;
	}
	node &operator* () {
		return *ptr;
	}
};
class const_node_iter { //结点的常量迭代器类型
private:
	const node *ptr;
public:
	const_node_iter(const node *data) : ptr(data) {}
	bool operator== (const_node_iter rhs) const {
		return ptr == rhs.ptr;
	}
	bool operator!= (const_node_iter rhs) const {
		return ptr != rhs.ptr;
	}
	void operator++ () {
		ptr = ptr->right_sum;
	}
	const node *operator-> () {
		return ptr;
	}
	const node &operator* () {
		return *ptr;
	}
};
class graph_t { //地图类
	friend class saver;
	friend class node;
	friend class debug_t;
	friend class kill_iter;
public:
#ifndef FINAL_RELEASE
	bool initialized;
#endif
	int strategy; //走棋策略（有正常、进攻、防守三种）
	node head; //结点的链表头
	long long grades[2]; //玩家和电脑各自的得分
	node nodes[SAVE_WIDTH][SAVE_WIDTH]; //地图结点集合
	stack<pos, SAVE_WIDTH * SAVE_WIDTH + 5> order; //结点栈（由于插销操作）
	unsigned int line[4/*direction*/][SAVE_WIDTH * 3/*state*/][2/*who*/];
	//bool closed[2][SAVE_WIDTH][SAVE_WIDTH];
public:
	inline int score(pos position, int direction, int id);
	inline int score(pos position, char type) { //计算在position处放type类型的棋子的得分
		int id = type == MACHINE ? MACHINE_ID : PERSON_ID;
		auto sc1 = score(position, 0, id);
		auto sc2 = score(position, 1, id);
		auto sc3 = score(position, 2, id);
		auto sc4 = score(position, 3, id);
		//printf("%d %d %d %d\n", sc1, sc2, sc3, sc4);
		return sc1 + sc2 + sc3 + sc4 + ADDITION4[RANK[sc1]][RANK[sc2]][RANK[sc3]][RANK[sc4]];
	}
	/*inline bool &unable(pos position, int id) {
		return closed[id][position.row][position.column];
	}*/
	inline graph_t &operator= (const graph_t &rhs) { //比较两个棋盘是否同构
		if (this != &rhs) {
			this->clear();
			this->strategy = rhs.strategy;
			for (auto position : rhs.order) {
				if (rhs.get(position) == MACHINE)
					this->set(position, MACHINE_ID);
				else if (rhs.get(position) == PERSON)
					this->set(position, PERSON_ID);
			}
		}
		return *this;
	}
	inline pos operator[] (int idx) const { //获取棋盘栈中idx位置的结点
		return order[idx];
	}
public:
#ifndef FINAL_RELEASE
	graph_t() : initialized(false) {}
#else
	graph_t() = default;
#endif
	void print(pos mark = pos(-1, -1)) const; //打印当前棋盘（在控制台中输出）
	void undo(); //撤销上一次的走棋
	void debug(int seed = 6666); //调试棋盘
	void set(pos position, int id, bool adjust = true); //带地图中摆放棋子（位置为position，类型为id）
	inline void set(int row, int column, int id) {
		set(pos(row, column), id);
	}
	void clear() { //将棋盘清空
		//std::memset(this->closed, 0, sizeof(this->closed));
		while (!order.empty())
			undo();
	}
	//计算在位置position处摆放id类型的棋子可以得到的总得分
	inline int sum_score(pos position, int id) const {
		const node &nd = nodes[position.row][position.column];
		return nd.score_sum[id];
	}
	//计算位置position的总得分
	inline int sum_score(pos position) const {
		const node &nd = nodes[position.row][position.column];
		return nd.score_sum[0] + nd.score_sum[1];
	}
	//计算所有位置对于类型id的得分和
	inline long long sum_score(int id) const {
		long long ret = 0;
		for (int i = 1; i <= GRAPH_WIDTH; ++i) {
			for (int j = 1; j <= GRAPH_WIDTH; ++j) {
				const node &nd = nodes[i][j];
				if (nd.type == SPACE)
					ret += DUMP_SCORE(nd.score_sum[id]);
			}
		}
		return ret;
	}
	inline int max_score(pos position, int id) const {
		const node &nd = nodes[position.row][position.column];
		return nd.score_max[id];
	}
	inline int max_score(int id) const {
		if (head.right_max[id] == &head)
			return -1;
		return head.right_max[id]->score_max[id];
	}
	//获取棋盘中位置p处的棋子类型
	inline char get(pos p) const {
		return nodes[p.row][p.column].type;
	}
	//获取棋盘中位置(r, c)处的棋子类型
	inline char get(int r, int c) {
		return nodes[r][c].type;
	}
	inline wall_type get_wall_type(pos position, pos direct, char type) {
		position += direct;
		node &now = nodes[position.row][position.column];
		if (now.type == type)
			return wall_type::empty;
		else if (now.type != SPACE)
			return wall_type::just_wall;
		pos next_pos = position + direct;
		node &next = nodes[next_pos.row][next_pos.column];
		if (next.type == SPACE || next.type == type)
			return wall_type::empty;
		else
			return wall_type::space_wall;
	}
	inline node_iter begin() { //第一个位置结点的迭代器（用于循环）
		return node_iter(head.right_sum);
	}
	inline node_iter end() { //尾后位置的迭代器（由于循环）
		return node_iter(&head);
	}
	inline const_node_iter begin() const { //常量迭代器
		return const_node_iter(head.right_sum);
	}
	inline const_node_iter end() const {
		return const_node_iter(&head);
	}
	inline bool control(int id) const {
		if (max_score(id) > SUCCESS_1)
			return true;
		return false;
	}
	//根据玩家和电脑的等分来判断是否有一方已经获得胜利
	inline static bool success(int my_score, int another_score) {
		if ((my_score > SUCCESS_1) ||
			(my_score > SUCCESS_2 && another_score < SUCCESS_1) ||
			(my_score > SUCCESS_3 && another_score < SCORE_MAX_BOUND)
			)
			return true;
		return false;
	}
	//判断类型id是否获胜
	inline bool success(int id) {
		return success(max_score(id), max_score(id ^ 1));
	}
	//简单的返回一个当前得分最高的位置
	inline pos good_pos() const {
		return head.right_sum->location;
	}
	//判断类型为id的玩家是否进入必胜状态
	inline pos kill_pos(int id) const {
		return head.right_max[id]->location;
	}
	//返回当前对于类型位id的玩家的最优走棋位置
	inline pos best_pos(int id) {
		int my_score = max_score(id);
		int another_score = max_score(id ^ 1);
		if (success(my_score, another_score))
			return kill_pos(id);
		return good_pos();
	}
	//返回棋局的得分，得分越高对电脑越有利，越低对玩家越有利
	inline long long score() const {
		long long sc;
		switch (strategy) {
		case STRATEGY_DEFENSE:
			sc = grades[MACHINE_ID] - grades[PERSON_ID] * 2;
			break;
		case STRATEGY_ATTACK:
			sc = grades[MACHINE_ID] * 2 - grades[PERSON_ID];
			break;
		default:
			sc = grades[MACHINE_ID] - grades[PERSON_ID];
		}
#ifndef FINAL_RELEASE
		int sign = sc > 0 ? 1 : -1;
		return static_cast<long long>(sign * std::sqrt(std::abs(sc)));
#else
		return sc;
#endif
	}
	inline size_t size() const { //返回棋盘的大小
		return order.size();
	}
	inline bool empty() const { //判断棋盘是否为空
		return order.empty();
	}
	inline pos last() const { //返回上一次下的位置
		return order.top();
	}
	inline void maintain(node &self, pos direct, bool adjust = true);
	inline void maintain_single(node &self, pos direct, int i, bool adjust = true);
	inline void enter(node &self);
	inline void adjust_sum(node &self);
	inline void adjust_max(node &self, int id, node *L, node *R);
	inline void update_max(node &self, int id, int new_score);
	inline void back(saver &self);
	inline void resume(node &self) {
		while (!self.save.empty()) {
			back(self.save.top());
			self.save.pop();
		}
	}
	inline void set_strategy(int number) { //设置电脑玩家的走棋策略（正常、进攻、防守）
		this->strategy = number; 
	}
	void initialize();
};
class debug_t { //用于调试棋盘的类
private:
	base_node nodes[SAVE_WIDTH][SAVE_WIDTH];
	const graph_t *owner;
public:
	inline debug_t() = default;
	inline debug_t(const graph_t *graph_ptr) : owner(graph_ptr) {
		for (int i = 0; i < SAVE_WIDTH; ++i) {
			for (int j = 0; j < SAVE_WIDTH; ++j) {
				nodes[i][j] = owner->nodes[i][j];
			}
		}
	}
	inline bool check() {
		for (int i = 0; i < SAVE_WIDTH; ++i) {
			for (int j = 0; j < SAVE_WIDTH; ++j) {
				if (nodes[i][j] != static_cast<base_node>(owner->nodes[i][j]))
					return false;
			}
		}
		return true;
	}
};
void graph_set(graph_t &map, int id, std::initializer_list<pos> list);
#endif
