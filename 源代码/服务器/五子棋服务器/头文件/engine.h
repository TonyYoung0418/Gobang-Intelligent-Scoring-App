#ifndef ENGINE_GUARD
#define ENGINE_GUARD
//��������ŵ����巽��
std::pair<int, pos> great_pos(const graph_t &map, bool time_check = false, int limit = 8);
//�������һ������λ��
pos get_next_place(const graph_t &map, bool warning, bool time_check = false);
void thread_test(); //�̲߳��Ժ���
void init_engine(); //��ʼ����������
void print_path(graph_t *map = nullptr);
int getScore(); //��õ�ǰ��ֵĵ÷�da
extern long long grades;
#endif
