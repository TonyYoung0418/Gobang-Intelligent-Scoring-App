#ifndef CONSTANT_GUARD //文件保护符
#define CONSTANT_GUARD
#define CONVERT_POS(p) CONVERT_BASE[p.row + 1][p.column + 1] //坐标转换
#define INVALID -1 //无效值
#define SCORE_UPPER_BOUND 200000 //得分上界
#define NUM_RANKS 7 //不同的等级个数
#define SCORE_MAX_BOUND (9000 + 2600 + 4500) //16100
#define SUCCESS_1 100000 //大胜得分
#define SUCCESS_2 34000 //小胜得分
#define SUCCESS_3 24000 //胜利得分
#define JOIN_WALL_TYPE(l, r) ((l) * 3 + (r)) //对墙类型进行合并
#define DUMP_SCORE(sc) ((long long)(sc) * (sc)) //计算得分的平方
#define DR(d) RANK[self.direct_score[i][d]] //计算方向d的得分
#define STATE_ARRAY_SIZE (1 << SAVE_WIDTH) //状态数组的大小
#define DIR_NUM0(r, c) (r) //方向1
#define DIR_NUM1(r, c) (c) //方向2
#define DIR_NUM2(r, c) ((r) - (c) + SAVE_WIDTH) //方向3
#define DIR_NUM3(r, c) r + c //方向4
#define COLUMN_WIDTH 25 //列宽
#define SCORE_CONTROL 9000 //控制得分
#define STRATEGY_NORMAL 0 //正常策略
#define STRATEGY_DEFENSE 1 //防守策略
#define STRATEGY_ATTACK 2 //进攻策略
#define STATE_NORMAL 0 //正常状态
#define STATE_SUCCESSFUL 1 //成功状态
#define STATE_WARNING 2 //警告状态
#define STATE_IN_DATABASE 3 //当前状态在数据库中有记录
#define MAX_STATE 128 //最大状态数
#define MAX_WALL 9 //最多的墙数
#ifndef FINAL_RELEASE
#define SCORE_DEFENSE -70000LL
#define SCORE_ATTACK 15000LL
#else
#define SCORE_DEFENSE (-70000LL * 70000LL) //防守得分
#define SCORE_ATTACK (15000LL * 15000LL) //进攻得分
#endif
const char PERSON = 'o'; //玩家下的棋在地图中对应的字符
const char MACHINE = 'w'; //电脑下的棋在地图中对应的字符
const char WALL = '#'; //墙在地图中字符
const char SPACE = ' '; //空白在地图中的字符
const char NONE = '!'; //特殊点再地图中的字符
const char STAR = '*'; //当前位置在地图中的字符
const int GRAPH_WIDTH = 15; //棋盘宽度
const int SAVE_WIDTH = GRAPH_WIDTH + 2; //棋盘保存宽度
const int GRAPH_SIZE = GRAPH_WIDTH * GRAPH_WIDTH; //整个棋盘的大小
const int PERSON_ID = 1, MACHINE_ID = 0; //玩家和计算机的编号
const int INF = 1 << 29; //无穷大
const long long INF_LONG = 1LL << 61; //无穷大
const char CHARACTER[] = { MACHINE , PERSON };
enum wall_type { empty = 0, space_wall = 1, just_wall = 2 };
enum ascii { up = 72, down = 80, left = 75, right = 77, enter = 13 };
extern unsigned int STATE_RANGE[STATE_ARRAY_SIZE][SAVE_WIDTH]; //状态范围
extern int STATE_START[STATE_ARRAY_SIZE][SAVE_WIDTH]; //起始状态
extern int RANGE_LENGTH[STATE_ARRAY_SIZE]; //范围的长度
extern int STATE_LENGTH[STATE_ARRAY_SIZE]; //状态的长度
extern int BIT_REVERSE[STATE_ARRAY_SIZE]; //按位反转
extern int CONVERT_BASE[3][3]; //转换基
extern int RANK[SCORE_UPPER_BOUND]; //得分的等级
extern int ADDITION2[NUM_RANKS][NUM_RANKS]; //两个方向得分组合后的得分
extern int ADDITION4[NUM_RANKS][NUM_RANKS][NUM_RANKS][NUM_RANKS]; //四个方向得分组合后的得分
extern int NUMSPACES[STATE_ARRAY_SIZE]; //空格的数量
extern int NUMTHINGS[STATE_ARRAY_SIZE]; //非空白的数量
extern int SCORE_TABLE[GRAPH_WIDTH/*状态的长度-1*/][1 << GRAPH_WIDTH/*状态*/][GRAPH_WIDTH/*要估分的位置-1*/];
extern int INDEX[4/*行 列 主 副*/][SAVE_WIDTH][SAVE_WIDTH];
extern int DIR_NUM[4/*行 列 主 副*/][SAVE_WIDTH][SAVE_WIDTH];
extern int WEIGHT[MAX_STATE][MAX_WALL]; //状态的权重
extern void init_constant(); //初始化所有的常量
#endif