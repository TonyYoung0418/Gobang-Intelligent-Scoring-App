#include "stdafx.h"
#define HASH_NUM 2000700033 //哈希数
#define LEFT_MOVE_NUM 32 //组合状态时左移得到位数
#define TABLE_HEAD_SIZE 10000003 //哈希表表头的大小
#define TABLE_SIZE 2000003 //哈希表的大小
#define START_DEPTH 0 //起始搜索深度
#define BOUND true
#define DEFINITE false
#define START_CUTTING 20
#define TO_CHAR(data, player) reinterpret_cast<unsigned char *>(&data[player])
#define TO_CHAR_CONST(data, player) reinterpret_cast<const unsigned char *>(&data[player])
#define MAX_THREAD_NUMBER 3 //最大线程数量
#define SUCCESS(sc) ((sc) == INF_LONG) //分数sc是否表示已经获胜
#define MAX_FILE_SIZE 30000000 //文件大小上限
#define IS_LEAF(x) ((x) & 1) //是否是叶结点
#define HAS_BROTHER(x) ((x) & (1 << 1)) //结点x是否有兄弟
#define IS_GOOD(x) ((x) & (1 << 2)) //是否是必胜状态
#define IS_BAD(x) ((x) & (1 << 3)) //是否是必败状态
#define IS_FORBIDDEN(x) ((x) & (1 << 4)) //是否为禁手状态
#define NUM_DIRECTIONS 8 //所有可能的旋转方向
#define NUM_NODES 10000000 //最大结点数量
#define WORD(x) *((int *)(&buffer[x])) //读取一个字
#define MAX_PATH_SEARCH_DEPTH 10
struct Node;
template<int idx> class point;
using result_t = std::pair<long long, pos>;
long long grades = 0; //棋局得分 
const pos middle(GRAPH_WIDTH / 2 + 1, GRAPH_WIDTH / 2 + 1); //棋盘中心位置
//由于棋盘总共有4个对称轴且90度的旋转等价，也就意味着 对整个棋盘做x,y->(+/-)x,(+/-)y | (+/-)y,(+/-)x变换不改变棋局情况，变换前后状态等价
std::function<pos(pos)> right_rotate_array[NUM_DIRECTIONS] = { //右旋函数数组
	[](pos place) ->pos { return{ place.row, place.column }; }, //(x,y)不变
	[](pos place) ->pos { return { -place.row, place.column }; }, //(-x,y)
	[](pos place) ->pos { return{ place.row, -place.column }; }, //(x,-y)
	[](pos place) ->pos { return{ -place.row, -place.column }; }, //(-x,-y)
	[](pos place) ->pos { return{ place.column, place.row }; }, //(y,x)
	[](pos place) ->pos { return{ -place.column, place.row }; }, //(y,-x)
	[](pos place) ->pos { return{ place.column, -place.row }; }, //(-y,x)
	[](pos place) ->pos { return{ -place.column, -place.row }; }, //(-y,-x)
};
std::function<pos(pos)> left_rotate_array[NUM_DIRECTIONS] = { //左旋函数数组
	[](pos place) ->pos { return{ place.row, place.column }; }, //(x,y)不变
	[](pos place) ->pos { return{ -place.row, place.column }; }, //(-x,y)
	[](pos place) ->pos { return{ place.row, -place.column }; }, //(x,-y)
	[](pos place) ->pos { return{ -place.row, -place.column }; },//(-x,-y)
	[](pos place) ->pos { return{ place.column, place.row }; }, //(y,x)
	[](pos place) ->pos { return{ place.column, -place.row }; }, //(y,-x)
	[](pos place) ->pos { return{ -place.column, place.row }; }, //(-y,x)
	[](pos place) ->pos { return{ -place.column, -place.row }; }, //(-y,-x)
};
pos struggle[2];
graph_t graph[MAX_THREAD_NUMBER];
char* buffer;
size_t now, file_size;
//bool expansion;
std::vector<pos> path[2];
stack<pos, GRAPH_SIZE> list;
class state_t { //压缩的棋盘状态类，部分状态在经过旋转或对称操作后是完全等价的
	friend std::ostream& operator<< (std::ostream& os, const state_t& st);
private:
	unsigned long long data[2];
public:
	using hash_t = std::pair<unsigned int, state_t>;
	inline void insert(int depth, pos position) { //在深度depth处插入position
		TO_CHAR(data, depth & 1)[depth / 2] = position.row * GRAPH_WIDTH + position.column;
	}
	inline void undo(int depth) { //撤销depth处的操作
		TO_CHAR(data, depth & 1)[depth / 2] = 0;
	}
	inline hash_t hash() const { //返回自身的哈希值
		hash_t ret;
		ret.second = *this;
		auto* ptr = ret.second.data;
		std::sort(TO_CHAR(ptr, 0), TO_CHAR(ptr, 0) + sizeof(unsigned long long));
		std::sort(TO_CHAR(ptr, 1), TO_CHAR(ptr, 1) + sizeof(unsigned long long));
		auto join = ((ptr[0] % HASH_NUM) << LEFT_MOVE_NUM) | (ptr[1] % HASH_NUM);
		ret.first = join % TABLE_HEAD_SIZE;
		return ret;
	}
	inline void clear() { //清空当前对象
		memset(data, 0, sizeof(data));
	}
}state[MAX_THREAD_NUMBER];
std::ostream& operator<< (std::ostream& os, const state_t& st) { //输出状态st，即当期棋盘情况
	os << '[';
	for (int i = 0; i < 2; ++i) {
		if (i)
			os << ", ";
		os << '(';
		for (int j = 0; j < sizeof(unsigned long long); ++j) {
			if (j)
				os << ", ";
			os << static_cast<int>(TO_CHAR_CONST(st.data, i)[j]);
		}
		os << ')';
	}
	os << ']';
	return os;
}
class hash_table_t { //快速哈希表类
public:
	//使用表挂表的方式解决哈希冲突问题,使用数组模拟的方式实现链表
	unsigned int first[TABLE_HEAD_SIZE]; //表头数组
	unsigned int next[TABLE_SIZE]; //链表指针数组
	state_t data[TABLE_SIZE]; //状态数组
	long long value[TABLE_SIZE]; //得分数组
	int depth[TABLE_SIZE]; //深度数组
	pos kill[TABLE_SIZE]; //绝杀数组
	bool type[TABLE_SIZE]; //类型数组
	unsigned int now; //有效值从1开始
public:
	inline void clear() { //清空哈希表
		now = 0;
		std::memset(first, 0, sizeof(first));
	}
	template<int number> inline point<number> extract(const state_t::hash_t& hasher);
	inline int size() const { //返回哈希表的大小
		int ret = 0;
		for (int i = 0; i < TABLE_HEAD_SIZE; ++i) {
			int u = first[i];
			while (u) {//判断是否有下一个节点
				ret += 1;
				u = next[u];
			}
		}
		return ret;
	}
	inline int statistics() const { //统计哈希表的最大桶大小，即哈希表下挂链表中元素最多的那个表中的元素数量
		int ret = 0;
		for (int i = 0; i < TABLE_HEAD_SIZE; ++i) {//遍历哈希表
			int sum = 0;//记录当前下挂链表中元素数量
			int u = first[i];
			while (u) {//判断是否有下一个节点
				sum += 1;
				u = next[u];
			}
			ret = std::max(ret, sum);//取最大值
		}
		return ret;
	}
}hash_table[MAX_THREAD_NUMBER];
struct Node { //棋谱中的结点类
public:
	pos location; //坐标
	Node* child; //左孩子指针
	Node* brother; //右兄弟指针
	int direction;
	unsigned int flag;
public:
	Node() = default;
}nodes[NUM_NODES], * cur = nodes, * root, * prior;
Node* new_node(int r, int c) { //创建一个新的结点行号r，列号c
	cur->location = { r, c };
	cur->child = cur->brother = nullptr;
	cur->direction = 0;
	cur->flag = 0;
	return cur++;
}
template<int number> class point { //哈希表中的一个点
	friend std::ostream& operator<< (std::ostream& os, const point& info);
private:
	const int index;
public:
	inline point(int idx) : index(idx) {} //默认构造
	//用给定的值更新哈希表
	inline void update(pos kill, int depth, long long value, bool type) {
		hash_table[number].kill[index] = kill;
		hash_table[number].depth[index] = depth;
		hash_table[number].value[index] = value;
		hash_table[number].type[index] = type;
	}
	inline bool done() const { //当前值是否已经计算过
		return hash_table[number].depth[index] != INVALID;
	}
	inline long long value() const { //当前位置的值
		return hash_table[number].value[index];
	}
	inline pos kill() const { //当前位置是否是绝杀状态
		return hash_table[number].kill[index];
	}
	inline int depth() const { //当前状态对应的深度
		return hash_table[number].depth[index];
	}
	inline bool type() const { //状态的类型
		return hash_table[number].type[index];
	}
};
std::ostream& operator<< (std::ostream& os, const Node& nd) {//输出节点的行列数
	os << '(' << nd.location.row << ", " << nd.location.column << ')';
	return os;
}
template<int number> std::ostream& operator<< (std::ostream& os, const point<number>& info) {//输出搜索情况和当前状态的相关信息
	os << "Done: " << (hash_table[number].depth[info.index] != INVALID) << std::endl;
	os << "Kill: " << hash_table[number].kill[info.index] << std::endl;
	os << "Value: " << hash_table[number].value[info.index] << std::endl;
	os << "Depth: " << hash_table[number].depth[info.index] << std::endl;
	return os;
}
template<int number> inline point<number> hash_table_t::extract(const state_t::hash_t& hasher) {
	unsigned int u = first[hasher.first];
	while (u) {
		if (std::memcmp(&data[u], &hasher.second, sizeof(state_t)) == 0)
			return point<number>(u);
		u = next[u];
	}
	now += 1;
#ifndef FINAL_RELEASE
	if (now >= TABLE_SIZE)
		warn("哈希表溢出。");
#endif
	next[now] = first[hasher.first];
	first[hasher.first] = now;
	data[now] = hasher.second;
	depth[now] = INVALID;
	return point<number>(now);
}
template<int number> long long alphabeta(const int player, long long alpha, long long beta, const int depth, const int limit) {
	graph_t& graph = ::graph[number];
	state_t& state = ::state[number];
	hash_table_t& hash_table = ::hash_table[number];
	const int rest = limit - depth;
	if (graph.success(player)) {//如果胜句已定
		if (depth == START_DEPTH)
			hash_table.extract<number>(state.hash()).update(graph.best_pos(player), rest, INF_LONG, DEFINITE);
		//如果person获胜则应选择其获胜步数最多的着法
		return !player ? INF_LONG : (-INF_LONG + depth + 1);
	}
	if (depth >= limit) //达到深度限制则直接返回棋局分值
		return graph.score();
	if (graph.control(!player)) { //对方将军，此时只需考虑对方是否一步获胜，对于自身获胜已在第一个if处判断
		pos control = graph.kill_pos(!player);
		graph.set(control, player);
		state.insert(depth, control);
		long long result = alphabeta<number>(!player, alpha, beta, depth + 1, limit);
		graph.undo();
		state.undo(depth);
		if (depth == START_DEPTH)
			hash_table.extract<number>(state.hash()).update(control, rest, result, DEFINITE);
		return result; //将状态插入到哈希表
	}
	int counter = 0;
	bool type = BOUND; // mark = false;
	long long grades = !player ? -INF_LONG : INF_LONG;
	const auto hasher = state.hash();
	auto info = hash_table.extract<number>(hasher);
	if (info.done() && info.depth() >= rest) {
		if (info.type() == DEFINITE)
			return info.value();
		if (!player)
			alpha = std::max(alpha, info.value());
		else
			beta = std::min(beta, info.value());
		if (beta <= alpha)
			return !player ? alpha : beta;
	}
	pos kill_pos = info.done() ? info.kill() : graph.good_pos();
	/*if (expansion) {
		if (player == PERSON_ID && struggle[0].valid() && graph.get(struggle[0]) == SPACE && graph.get(struggle[1]) == SPACE) { //&& !graph.unable(struggle[0], PERSON_ID))
			//mark = true;
			for (int i = 0; i <= 1; ++i) {
				graph.set(struggle[i], PERSON_ID);
				graph.set(struggle[i ^ 1], MACHINE_ID);
				state.insert(depth, struggle[i]);
				state.insert(depth + 1, struggle[i ^ 1]);
				long long struggle_score = alphabeta<number>(player, alpha, beta, depth + 2, limit + 2);
				graph.undo();
				graph.undo();
				state.undo(depth + 1);
				state.undo(depth);
				beta = std::min(beta, struggle_score);
				if (struggle_score < beta) {
					beta = grades = struggle_score;
					kill_pos = struggle[i];
					type = DEFINITE;
				}
				if (beta <= alpha) {
					type = BOUND;
					goto end;
				}
			}
			//graph.unable(struggle[0], PERSON_ID) = true;
			//graph.unable(struggle[1], PERSON_ID) = true;
		}
	}*/ { //if (!graph.unable(kill_pos, player))
		graph.set(kill_pos, player);
		state.insert(depth, kill_pos);
		long long kill_score = alphabeta<number>(!player, alpha, beta, depth + 1, limit);
		graph.undo();
		state.undo(depth);
		if (!player)
			alpha = std::max(alpha, kill_score);
		else
			beta = std::min(beta, kill_score);
		if (beta <= alpha)
			goto end;
	}
	for (node& nd : graph) {
		if (++counter > COLUMN_WIDTH)
			break;
		pos position = nd.where();
		if (position == kill_pos)
			continue;
		//if (graph.unable(position, player))
		//	continue;
		graph.set(position, player, depth != limit - 1); //如果到达搜索的最后一层结点则不对score_sum链表进行调整
		state.insert(depth, position);
		long long result = alphabeta<number>(!player, alpha, beta, depth + 1, limit);
		graph.undo();
		state.undo(depth);
		if (!player && result > alpha) {
			alpha = grades = result;
			kill_pos = position;
			type = DEFINITE;
		}
		else if (player && result < beta) {
			beta = grades = result;
			kill_pos = position;
			type = DEFINITE;
		}
		if (beta <= alpha) {
			type = BOUND;
			break;
		}
	}
end:
	/*if (mark) {
		graph.unable(struggle[0], PERSON_ID) = false;
		graph.unable(struggle[1], PERSON_ID) = false;
	}*/
	info.update(kill_pos, rest, grades, type);
	return !player ? alpha : beta;
}
template<int number> inline void success_check(pos& place, long long& grades) {// 检索是否属于成功态
	graph_t& graph = ::graph[number];
	state_t& state = ::state[number];
	hash_table_t& hash_table = ::hash_table[number];
	if (graph.success(MACHINE_ID) && !graph.control(MACHINE_ID)) {
		graph.set(place, MACHINE_ID);
		for (node& him : graph) {
			bool ok = false;
			graph.set(him.where(), PERSON_ID);
			if (graph.success(MACHINE_ID)) {
				graph.undo();
				continue;
			}
			for (node& me : graph) {
				graph.set(me.where(), MACHINE_ID);
				bool result = graph.success(PERSON_ID);
				graph.undo();
				if (!result) {
					ok = true;
					break;
				}
			}
			graph.undo();
			if (!ok) {
				place = him.where();
				grades -= 1;
				break;
			}
		}
		graph.undo();
	}
}
template<int number> inline std::pair<long long, pos> search_thread_first(int limit = 8) {
	graph_t& graph = ::graph[number];
	state_t& state = ::state[number];
	hash_table_t& hash_table = ::hash_table[number];
	int max_depth = graph.size() < 4 ? 6 : 8;
	max_depth = std::min(max_depth, limit);

	hash_table.clear();
	state.clear();
	if (max_depth < 4)
		return{ 0, pos(-1, -1) };
	long long grades = 0;
	for (int limit = 2; limit <= max_depth - 2; limit += 2) //迭代加深搜索
		grades = alphabeta<number>(MACHINE_ID, -INF_LONG, INF_LONG, START_DEPTH, limit);
	auto information = hash_table.extract<number>(state.hash());
	pos place = information.kill();
	success_check<number>(place, grades);
	return{ grades, place };
}
template<int number> inline std::pair<long long, pos> search_thread_second(int limit = 8) {
	graph_t& graph = ::graph[number];
	state_t& state = ::state[number];
	hash_table_t& hash_table = ::hash_table[number];
	int max_depth = graph.size() < 4 ? 6 : 8;
	max_depth = std::min(max_depth, limit);

	long long grades = alphabeta<number>(MACHINE_ID, -INF_LONG, INF_LONG, START_DEPTH, max_depth);
	auto information = hash_table.extract<number>(state.hash());
	pos place = information.kill();
	success_check<number>(place, grades);
	return{ grades, place };
}
template<int number> inline std::pair<long long, pos> search_thread(int limit = 8) {
	search_thread_first<number>(limit);
	return search_thread_second<number>(limit);
}
inline void get_killer(graph_t& map, int id, pos location[2]) {
	for (auto& nd : map) {
		pos place = nd.where();
		if (nd.score() >= SCORE_CONTROL) {
			map.set(place, id);
			if (map.control(id)) {
				location[0] = place;
				location[1] = map.kill_pos(id);
			}
			map.undo();
		}
		if (nd.score() < SCORE_CONTROL || location[0].valid())
			break;
	}
}
pos get_next_place(const graph_t& map, bool warning, bool time_check) {
	auto result = great_pos(map, time_check);
	if (result.first && warning)
		warn("Success!");
	return result.second;
}
//测试多线程的效果，测试阶段使用，在实际使用时不调用该函数
void thread_test() {
	graph[0].clear();
	//同时测试多种可能的走法
	graph[0].set(pos(7, 7), MACHINE_ID);
	graph[0].set(pos(7, 6), MACHINE_ID);
	graph[0].set(pos(8, 7), PERSON_ID);
	graph[0].set(pos(7, 8), PERSON_ID);
	graph[0].set(pos(6, 9), PERSON_ID);
	graph[0].set(pos(9, 6), MACHINE_ID);
	graph[1].clear();
	graph[1].set(pos(8, 8), MACHINE_ID);
	graph[1].set(pos(8, 9), PERSON_ID);
	graph[1].set(pos(7, 9), MACHINE_ID);
	graph[1].set(pos(7, 8), PERSON_ID);
	std::clock_t start = std::clock();
	printf("开始多线程计算...\r");
	auto future0 = std::async(std::launch::async, search_thread<0>, 8);
	auto future1 = std::async(std::launch::async, search_thread<1>, 8);
	auto result0 = future0.get();
	auto result1 = future1.get();
	std::clock_t over = std::clock();
	if (result0.first != 20563 || result0.second != pos(8, 5) || result1.first != -18546 || result1.second != pos(6, 7))
		print("多线程计算错误！！！！！！");
	printf("多线程计算正确，用时：%f秒\n", static_cast<double>(over - start) / CLOCKS_PER_SEC);
}
void print_path(graph_t* map) {
	int depth = 0;
	int player = MACHINE_ID;
	for (;;) {
		const auto hasher = state[0].hash();
		auto info = hash_table[0].extract<0>(hasher);
		if (!info.done())
			break;
		state[0].insert(depth, info.kill());
		graph[0].set(info.kill(), player);
		graph[0].print();
		depth += 1;
		player ^= 1; //修改该状态下一步的行动方
	}
	if (map)
		*map = graph[0];
	graph[0].clear();
	std::memset(&state[0], 0, sizeof(state[0]));
}
void print_brother(Node* nd) { //输出结点nd的右兄弟
	print("--------------------------");
	while (nd != nullptr) {
		print(*nd);
		nd = nd->brother;
	}
}
void print_flag(int flag) {
#ifndef FINAL_RELEASE
	std::cout << "必胜：" << static_cast<bool>(IS_GOOD(flag)) << std::endl;//必胜信息
	std::cout << "必败：" << static_cast<bool>(IS_BAD(flag)) << std::endl;//必败信息
	std::cout << "禁手：" << static_cast<bool>(IS_FORBIDDEN(flag)) << std::endl;//禁手信息
#endif
}
void print_flag(Node* nd) {//打印标志
	print_flag(nd->flag);
}
void print_seq(pos seq[], int size) {//打印队列
	graph_t* map = new graph_t;
	map->initialize();
	for (int i = 0; i < size; ++i)
		map->set(seq[i], i % 2 == 0 ? MACHINE_ID : PERSON_ID);
	map->print();
	delete map;
}
pos random_good_brother(Node* nd) {
	std::vector<pos> choice;
	while (nd != nullptr) {
		if (IS_GOOD(nd->flag))
			choice.push_back(nd->location);
		nd = nd->brother;
	}
	if (choice.empty())
		return pos(-1, -1);
	std::srand((unsigned)std::time(nullptr));
	return choice[rand() % choice.size()];
}
Node* build_tree(int depth = 0) {//建树
	unsigned char data = buffer[now++];
	unsigned char flag = buffer[now++];
	int column = data & 0xF;
	int row = data >> 4;
	Node* nd = new_node(row, column);
	nd->flag = flag;
	if (!IS_LEAF(flag))
		nd->child = build_tree(depth + 1);
	if (HAS_BROTHER(flag))
		nd->brother = build_tree(depth);
	return nd;
}
Node* read_database(std::string filename) {//读取文件中的信息
	::buffer = new char[MAX_FILE_SIZE];
	FILE* fp = fopen(filename.c_str(), "rb");
	::file_size = fread(buffer, sizeof(char), MAX_FILE_SIZE, fp);
	fclose(fp);
	::now = 0;
	::root = build_tree();
	::prior = new_node(-1, -1);
	::prior->child = root;
	::prior->brother = nullptr;
	while (now < file_size) {
		int x = WORD(now);
		now += sizeof(int);
		int y = WORD(now);
		now += sizeof(int);
		char dir = buffer[now];
		now += sizeof(char);
		nodes[x].child = &nodes[y];
		nodes[x].direction = dir;
	}
	delete buffer;
	return root;
}
//右旋函数组实现
inline pos right_rotate(pos position, int direction) {
	position -= middle;
	position = right_rotate_array[direction](position);
	return position + middle;
}
inline void right_rotate(std::vector<pos>& vec, int direction) {
	for (auto& position : vec)
		position = right_rotate(position, direction);
}
//左旋函数组实现
inline pos left_rotate(pos position, int direction) {
	position -= middle;
	position = left_rotate_array[direction](position);
	return position + middle;
}
inline void left_rotate(std::vector<pos>& vec, int direction) {
	for (auto& position : vec)
		position = left_rotate(position, direction);
}
inline pos right_rotate(pos position, int directions[], int length) {
	for (int i = 0; i < length; ++i)
		position = right_rotate(position, directions[i]);
	return position;
}
inline pos left_rotate(pos position, int directions[], int length) {
	for (int i = length - 1; i >= 0; --i)
		position = left_rotate(position, directions[i]);
	return position;
}
std::pair<pos, int> step_by_tree(pos sequence[], int size, Node* tree) {//位置信息与bottom
	int bottom = 0;
	for (int i = 0; i < NUM_DIRECTIONS; ++i) {
		Node* nd = tree;
		int directions[GRAPH_SIZE], length = 1;
		directions[0] = i;
		for (int j = 0; j < size; ++j) {
			pos place = sequence[j];
			pos position = right_rotate(place, directions, length);
			while (nd && nd->location != position)
				nd = nd->brother;
			if (!nd)
				break;
			if (nd->direction)
				directions[length++] = nd->direction;
			nd = nd->child;
			bottom = std::max(bottom, j + 1);
		}
		if (nd) {
			pos selection = random_good_brother(nd);
			if (selection.valid())
				return{ left_rotate(selection, directions, length), bottom + 1 };
		}
	}
	return{ pos(-1, -1), bottom };
}
bool search_state(Node* nd, int bottom, int depth = 0) {//状态搜索
	if (depth == bottom && IS_GOOD(nd->flag))
		return true;
	for (Node* child = nd->child; child; child = child->brother) {
		if (child->direction)
			continue;
		auto& sequence = path[depth % 2];
		if (std::find(sequence.begin(), sequence.end(), child->location) != sequence.end()) {
			list.push(child->location);
			if (search_state(child, bottom, depth + 1))
				return true;
			list.pop();
		}
	}
	return false;
}
/*选择走法
1.体现难度
2.在较高难度下确定选择解的优秀性
3.剪枝，每步计算所需要的时间可以接受
4.添加一定的随机性，使得棋局并非千篇一律
*/
std::pair<int, pos> great_pos(const graph_t& map, bool time_check, int limit) { //limit是对每个状态的搜索数量，默认值为8，即周围相邻的8个点
	if (map.empty()) {//特判空棋盘，即刚开局，此时必是电脑先手
		::grades = 0;//空棋盘得分当然 = 0
		return std::make_pair(STATE_NORMAL, middle);//先手第一步稳定放在中间
	}
	else if (map.size() == 1) {//特判第一步，即person刚走了第一步，此时必是电脑后手
		::grades = -299999;
		std::vector<pos> sequence;
		pos the = map.last();
		for (pos position : range(the, 8))//相邻的8个位置
			sequence.push_back(position);
		std::srand((unsigned)std::time(nullptr));
		std::random_shuffle(sequence.begin(), sequence.end());
		if (the == middle) //如果下在棋盘中间则随机选周围一个位置,加入此随机性以使得不是每次走法都是一样的
			return std::make_pair(STATE_NORMAL, sequence.front());
		std::sort(sequence.begin(), sequence.end(), [](const pos& p1, const pos& p2) {
			auto d1 = (p1 - middle).length();
			auto d2 = (p2 - middle).length();
			return d1 < d2;
			});//排序后选择离中心最近的位置
		return std::make_pair(STATE_NORMAL, sequence.front());
	}
	if (limit > 8) {//搜索位置限制大于8，可能去尝试搜索一些其它位置
		::grades = 1000;
		pos offset(0, 0);
		pos sequence[GRAPH_SIZE];
		int length = 0;
		path[0].clear();
		path[1].clear();
		for (pos position : map.order)
			sequence[length++] = position;
		if (map.size() >= 2) {//如过仅开局/开局一步，无需扩大搜索
			//一些常用的搜索点
			const pos A2(6, 8) /*天元向上2格*/, A3(7, 7) /*蒲月*/, offA(9, 9) /*偏移量(1, 1)*/;
			const pos B2(6, 7), B3(7, 6) /*水月*/, offB(9, 10) /*偏移量(1, 2)*/;
			const pos place = map[1];
			for (int i = 0; i < NUM_DIRECTIONS; ++i) {
				if (right_rotate(A2, i) == place) {
					if (map.size() == 2) {
						return{ STATE_IN_DATABASE, right_rotate(A3, i) };
					}
					else {
						offset = right_rotate(offA, i) - middle;
						std::swap(sequence[0], sequence[2]);
						break;
					}
				}
			}
			for (int i = 0; i < NUM_DIRECTIONS; ++i) {
				if (right_rotate(B2, i) == place) {
					if (map.size() == 2) {
						return{ STATE_IN_DATABASE, right_rotate(B3, i) };
					}
					else {
						offset = right_rotate(offB, i) - middle;
						std::swap(sequence[0], sequence[2]);
						break;
					}
				}
			}
		}
		for (int i = 0; i < length; ++i)
			sequence[i] += offset;
		auto result = step_by_tree(sequence, length, root);
		pos killer = result.first;
		int bottom = result.second;
		//扩大搜索
		if (!killer.valid() && bottom <= MAX_PATH_SEARCH_DEPTH) {
			for (int i = 0; i < bottom; ++i)
				path[i % 2].push_back(sequence[i]);
			if (bottom == length) {
				path[bottom % 2].emplace_back();
				for (int direction = 0; direction < NUM_DIRECTIONS; ++direction) {
					right_rotate(path[0], direction);
					right_rotate(path[1], direction);//旋转,避免重复，减少搜索
					for (int i = 3; i <= GRAPH_WIDTH - 2; ++i) {
						for (int j = 3; j <= GRAPH_WIDTH - 2; ++j) {
							pos place = pos(i, j);
							if (std::find(path[0].begin(), path[0].end(), place) != path[0].end() ||
								std::find(path[1].begin(), path[1].end(), place) != path[1].end())
								continue;
							list.clear();
							path[bottom % 2].back() = pos(i, j);
							if (search_state(prior, length + 1)) {
								return{ STATE_IN_DATABASE, left_rotate(place, direction) - offset };
							}
						}
					}
					left_rotate(path[0], direction);
					left_rotate(path[1], direction);
				}
			}
			else if (bottom < length) {
				path[bottom % 2].push_back(sequence[bottom]);
				for (int direction = 0; direction < NUM_DIRECTIONS; ++direction) {
					right_rotate(path[0], direction);
					right_rotate(path[1], direction);//旋转,避免重复，减少搜索
					list.clear();
					if (search_state(prior, bottom + 1)) {
						for (size_t i = 0; i < list.size(); ++i)
							sequence[i] = left_rotate(list[i], direction); //对前bottom+1个位置的顺序进行更改
						killer = step_by_tree(sequence, length, root).first;
						break;
					}
					left_rotate(path[0], direction);
					left_rotate(path[1], direction);
				}
			}
		}
		if (killer.valid())
			return{ STATE_IN_DATABASE, killer - offset };
	}
	//else
	//	expansion = false;
	std::clock_t start = std::clock(); //计时器启动

	graph[0] = map;
	result_t selection;
	pos location[2] = { pos(-1, -1), pos(-1, -1) };
	//struggle[0] = struggle[1] = pos(-1, -1);
	get_killer(graph[0], MACHINE_ID, location);
	//get_killer(graph[0], PERSON_ID, struggle);
	if (limit <= 8 || !location[0].valid() || graph[0].control(PERSON_ID) || graph[0].control(MACHINE_ID)) {//无需再做额外处理
		selection = search_thread_first<0>(limit);
		if (!SUCCESS(selection.first)) {
			if (limit > 8) {//如果需要扩大搜索(即是后三个原因导致进入该分支）
				if (selection.first < SCORE_DEFENSE)
					graph[0].set_strategy(STRATEGY_DEFENSE);
				else if (selection.first > SCORE_ATTACK)
					graph[0].set_strategy(STRATEGY_ATTACK);
			}
			selection = search_thread_second<0>(limit);
		}
	}
	else {
		result_t result[3];
		//置入扩大搜索导致的，需要额外处理的内容
		graph[1] = graph[2] = map;
		graph[0].set(location[0], MACHINE_ID);
		graph[0].set(location[1], PERSON_ID);
		graph[1].set(location[1], MACHINE_ID);
		graph[1].set(location[0], PERSON_ID);
		//graph[2].unable(location[0], MACHINE_ID) = true;
		//graph[2].unable(location[1], MACHINE_ID) = true;
		auto future0 = std::async(search_thread_first<0>, limit);
		auto future1 = std::async(search_thread_first<1>, limit);
		result[2] = search_thread_first<2>(limit);
		result[1] = future1.get();
		result[0] = future0.get();
		//printf("%lld %lld %lld\n", result[0].first, result[1].first, result[2].first);
		selection = *std::max_element(result, std::end(result),
			[](result_t r1, result_t r2) {
				return r1.first < r2.first;
			});
		//对额外信息进行处理
		if (!SUCCESS(selection.first)) {
			if (selection.first < SCORE_DEFENSE) {
				for (int i = 0; i < 3; ++i)
					graph[i].set_strategy(STRATEGY_DEFENSE);
			}
			else if (selection.first > SCORE_ATTACK) {
				for (int i = 0; i < 3; ++i)
					graph[i].set_strategy(STRATEGY_ATTACK);
			}
			future0 = std::async(search_thread_second<0>, limit);
			future1 = std::async(search_thread_second<1>, limit);
			result[2] = search_thread_second<2>(limit);
			result[1] = future1.get();
			result[0] = future0.get();
		}
		result[0].second = location[0];
		result[1].second = location[1];
		selection = *std::max_element(result, std::end(result),
			[](result_t r1, result_t r2) {
				return r1.first < r2.first;
			});
		if (SUCCESS(result[2].first))
			selection.second = result[2].second;
	}
	::grades = selection.first;
	pos place = selection.second;
	bool successful = SUCCESS(grades);
	std::clock_t over = std::clock();

	if (time_check) {
		printf("Alpha-Beta搜索用时：%f秒\n", static_cast<double>(over - start) / CLOCKS_PER_SEC);
		printf("得分: %lld\n", grades);
		printf("状态总数：%d\n", hash_table[0].size());
		printf("状态分布：%d\n", hash_table[0].statistics());
	}
	if (successful)
		return std::make_pair(STATE_SUCCESSFUL, place);
	else if (grades > SCORE_ATTACK)
		return std::make_pair(STATE_WARNING, place);
	else
		return std::make_pair(STATE_NORMAL, place);
}
void init_engine() { //初始化游戏引擎
	static bool have_called = false;//静态变量，第一次赋值时生效
	if (!have_called)//保证 have_called 为true
		have_called = true;
	else
		return;
	for (int i = 0; i < MAX_THREAD_NUMBER; ++i)
		graph[i].initialize();
	read_database("database.db");
}
int getScore() {//获取当前状态得分
	double score;
	if (abs(grades) <= 10)
		score = 0;
	else if (grades > 0)
		score = std::pow(grades, 0.19);
	else
		score = -std::pow(-grades, 0.19);
	printf("%f\n", score);
	score = std::min(score, 100.0);
	score = std::max(score, -100.0);
	return -score;
}