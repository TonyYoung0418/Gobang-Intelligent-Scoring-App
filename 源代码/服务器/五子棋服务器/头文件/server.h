#pragma once
#include <vector>
#include <mutex>
using json = nlohmann::json;
class Server {
private:
    graph_t& graph = *new graph_t;
    httplib::Server server;
    std::mutex mutex;
public:
    Server() { //初始化服务器
        init_engine();
        graph.initialize();
        run();
    }
    ~Server() { 
        delete &graph; 
    }
    void run() { //开始监听
        using namespace httplib;
        using namespace nlohmann;
        //初始化服务器
        server.Post("/server", [&](const Request& req, Response& res) {
            json input = json::parse(req.get_param_value("json"));
            json reply = response(input);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(reply.dump(), "application/json");
        });
        server.listen("0.0.0.0", 8888);
    }
    std::vector<std::vector<int>> check(char type) {  //检查type方是否获胜
        //枚举棋盘中的所有位置，看是否有连成5个棋子的位置
        for (int i = 1; i <= GRAPH_WIDTH; ++i) {
            for (int j = 1; j <= GRAPH_WIDTH; ++j) {
                std::vector<std::vector<int>> position;
                for (int k = 0; k < 5 && i + k <= GRAPH_WIDTH; ++k)
                    if (graph.get(i + k, j) == type)
                        position.push_back({i + k - 1, j - 1});
                if (position.size() >= 5) return position;
                position.clear();
                for (int k = 0; k < 5 && j + k <= GRAPH_WIDTH; ++k)
                    if (graph.get(i, j + k) == type)
                        position.push_back({i - 1, j + k - 1});
                if (position.size() >= 5) return position;
                position.clear();
                for (int k = 0;
                     k < 5 && j + k <= GRAPH_WIDTH && i + k <= GRAPH_WIDTH; ++k)
                    if (graph.get(i + k, j + k) == type)
                        position.push_back({i + k - 1, j + k - 1});
                if (position.size() >= 5) return position;
                position.clear();
                for (int k = 0; k < 5 && j - k >= 1 && i + k <= GRAPH_WIDTH;
                     ++k)
                    if (graph.get(i + k, j - k) == type)
                        position.push_back({i + k - 1, j - k - 1});
                if (position.size() >= 5) return position;
                position.clear();
            }
        }
        return {};
    }
    json response(json input) { //对输入input进行响应
        std::lock_guard<std::mutex> guard(mutex); //锁住线程锁
        std::cerr << input.dump() << std::endl; //输出接收到的json包
        json reply = {};
        graph.clear();
        //接下来从接受到的数据包中解析出棋盘的布局
        for (int index = 0, i = 1; i <= GRAPH_WIDTH; ++i) {
            for (int j = 1; j <= GRAPH_WIDTH; ++j) {
                int color = input["board"][index++];
                if (input["machine"] == color)
                    graph.set(i, j, MACHINE_ID);
                else if (input["person"] == color)
                    graph.set(i, j, PERSON_ID);
            }
        }
        //检查玩家是否获胜，若获胜则恢复构成胜局的5个棋子的位置
        auto position = check(PERSON);
        if (position.size() >= 5) {
            reply["result"] = "win";
            reply["loc"] = {position[0], position[1], position[2], position[3],
                            position[4]};
            return reply;
        }
        //解析出电脑玩家被设定成的难度
        int difficulty = input["difficulty"];  // 2 <= difficulty <= 10
        if (difficulty % 2 == 1) difficulty += 1;
        difficulty = std::max(difficulty, 2);
        difficulty = std::min(difficulty, 10);
        auto result = great_pos(graph, false, difficulty); //计算出电脑玩家的走棋位置
        int flag = result.first;
        pos place = result.second;
        graph.set(place, MACHINE_ID);
        reply["position"] = {place.row - 1, place.column - 1};
        reply["score"] = getScore();
        reply["result"] = "unknown";
        //判断是否为平局
        if (GRAPH_SIZE - graph.size() < 30 && !graph.control(PERSON_ID) &&
            flag != STATE_SUCCESSFUL) {
            reply["result"] = "tie";
        } else {
            auto position = check(MACHINE);
            if (position.size() >= 5) {
                reply["result"] = "lose";
                reply["loc"] = {position[0], position[1], position[2],
                                position[3], position[4]};
            }
        }
        return reply;
    }
};