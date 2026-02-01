#include <algorithm>
#include <iostream>
#include <memory>
#include <tuple>
#include <vector>

// 枚举，设置机器人类型
enum class RobotType {
    kInfantry = 0,
    kEngineer = 1
};

// 设置基类
class BaseRobot {
public:
    // 父类构造函数，赋值两种机器人通用的属性（队伍ID、机器人ID、机器人类别）
    BaseRobot(uint32_t team_id, uint32_t robot_id, RobotType type)
        : team_id(team_id), robot_id(robot_id), type(type), blood_(0), heat_(0),
          max_blood_(0), max_heat_(0), level_(1) {
    }

    // 虚析构函数，自动生成默认析构逻辑，防止从容器中移除时内存泄漏
    virtual ~BaseRobot() = default;

    // 获取队伍ID和机器人ID
    std::tuple<uint32_t, uint32_t> GetId() const {
        return {team_id, robot_id};
    }

    // 判断机器人是否击毁
    bool IsDead() const {
        if (blood_ <= 0) {
            return true;
        }
        return false;
    }

    // 随时间改变热量降低，超热量扣血
    void ChangeHeat(uint32_t time_delta) {
        // 判断热量改变后是否小于0,若小于0则直接将热量置0
        heat_ = (heat_ > time_delta) ? (heat_ - time_delta) : 0;
        // 当热量大于热量极限时，减少血量，判断血量减少后是否小于0,若小于0则置0
        if (heat_ > max_heat_) {
            blood_ = (blood_ > time_delta) ? (blood_ - time_delta) : 0;
        }
    }

    // 纯虚函数，使子类在不同状况下改变机器人属性
    virtual void Rebuild() =0;

    // 纯虚函数，使管理类可识别机器人类型
    virtual RobotType GetType() const = 0;

protected:
    // 机器人的所有属性，子类可访问，外部不可
    uint32_t team_id, robot_id, heat_, max_heat_, level_, max_blood_;
    RobotType type;

public:
    uint32_t blood_;
};

// 子类——步兵
class InfantryRobot : public BaseRobot {
public:
    InfantryRobot(uint32_t team_id, uint32_t robot_id) : BaseRobot(team_id, robot_id, RobotType::kInfantry) {
        level_ = 1;
        Rebuild();
    }

    // 步兵纯虚函数，根据等级不同机器人的属性赋值不同
    void Rebuild() override {
        heat_ = 0;
        // 根据等级赋值
        switch (level_) {
            case 1: {
                max_blood_ = 100;
                max_heat_ = 100;
                break;
            }
            case 2: {
                max_blood_ = 150;
                max_heat_ = 200;
                break;
            }
            case 3: {
                max_blood_ = 250;
                max_heat_ = 300;
                break;
            }
            default: {
                max_blood_ = 100;
                max_heat_ = 100;
            }
        }
        blood_ = max_blood_;
    }

    // 步兵升级
    bool Upgrade(uint32_t target_level) {
        if (target_level > level_ && target_level <= 3) {
            level_ = target_level;
            Rebuild();
            return true;
        }
        return false;
    }

    // 返回机器人类型
    RobotType GetType() const override {
        return RobotType::kInfantry;
    }

    // 步兵独有，热量增加
    void AddHeat(uint32_t add_heat) {
        heat_ += add_heat;
    }
};

// 子类——工程
class EngineerRobot : public BaseRobot {
public:
    EngineerRobot(uint32_t team_id, uint32_t robot_id) : BaseRobot(team_id, robot_id, RobotType::kEngineer) {
        Rebuild();
    }

    // 返回机器人类型
    RobotType GetType() const override {
        return RobotType::kEngineer;
    }

    // 工程纯虚函数，无热量，血量上限300
    void Rebuild() override {
        max_blood_ = 300;
        blood_ = max_blood_;
        heat_ = 0;
        max_heat_ = 0;
    }
};

//机器人管理类
class RobotManager {
private:
    //创建一个储存存活机器人的容器
    std::vector<std::shared_ptr<BaseRobot> > live_robots_;
    //创建一个储存已死亡机器人的容器
    std::vector<std::shared_ptr<BaseRobot> > dead_robots_;
    //初始化时间
    uint32_t last_time_ = 0;

public:
    //在活机器人容器中找机器人
    std::shared_ptr<BaseRobot> FindLiveRobot(uint32_t team_id, uint32_t robot_id) {
        //运用迭代器找双ID匹配的机器人
        auto it = find_if(live_robots_.begin(), live_robots_.end(), [&](const std::shared_ptr<BaseRobot> &robot) {
            return robot->GetId() == std::make_tuple(team_id, robot_id);
        });
        if (it != live_robots_.end()) {
            return *it;
        }
        return nullptr;
    }

    //在已击毁的容器中找机器人
    std::shared_ptr<BaseRobot> FindDeadRobot(uint32_t team_id, uint32_t robot_id, RobotType type) {
        //运用迭代器找双ID匹配的机器人
        auto it = find_if(dead_robots_.begin(), dead_robots_.end(), [&](const std::shared_ptr<BaseRobot> &robot) {
            return robot->GetId() == std::make_tuple(team_id, robot_id) && robot->GetType() == type;
        });
        if (it != dead_robots_.end()) {
            return *it;
        }
        return nullptr;
    }

    //处理时间变化函数
    void HandleTimeChange(uint32_t curr_time) {
        //如果时间不变，各参数不变，直接返回
        if (curr_time <= last_time_) return;
        uint32_t time_delta = curr_time - last_time_;
        //运用迭代器遍历活机器人，改变其参数
        for (auto it = live_robots_.begin(); it != live_robots_.end();) {
            (*it)->ChangeHeat(time_delta);
            //判断参数改变后是否死亡，若死亡则移到击毁池，并按格式输出
            if ((*it)->IsDead()) {
                dead_robots_.push_back(*it);
                auto [team_id,robot_id] = (*it)->GetId();
                std::cout << "D" << " " << team_id << " " << robot_id << std::endl;
                it = live_robots_.erase(it);
            } else {
                it++;
            }
        }
        last_time_ = curr_time;
    }

    //处理指令A，添加或复活机器人
    void HandleCommandA(uint32_t team_id, uint32_t robot_id, RobotType type) {
        //若机器人已存在且未死亡则指令无效返回
        if (FindLiveRobot(team_id, robot_id) != nullptr) return;
        //查找机器人是否在击毁池中，若在则复活
        auto robot = FindDeadRobot(team_id, robot_id, type);
        if (robot != nullptr) {
            robot->Rebuild();
            live_robots_.push_back(robot);
            //在击毁池中删除该机器人
            for (auto it = dead_robots_.begin(); it != dead_robots_.end();) {
                if ((*it)->GetId() == std::make_tuple(team_id, robot_id)) {
                    it = dead_robots_.erase(it);
                } else {
                    it++;
                }
            }
            return;
        }

        //若不在击毁池中，则按类别新建该机器人
        if (type == RobotType::kInfantry) {
            live_robots_.push_back(std::make_shared<InfantryRobot>(team_id, robot_id));
        } else if (type == RobotType::kEngineer) {
            live_robots_.push_back(std::make_shared<EngineerRobot>(team_id, robot_id));
        }
    }

    //处理F指令，机器人扣血指令
    void HandleCommandF(uint32_t team_id, uint32_t robot_id, uint32_t damage) {
        //在存活机器人中找该机器人
        auto robot = FindLiveRobot(team_id, robot_id);
        //如果没找到或者已击毁，则指令无效返回
        if (robot == nullptr || robot->IsDead()) return;
        //更新血量，如果掉血量高于现有血量直接归零
        uint32_t new_blood = robot->blood_ > damage ? (robot->blood_ - damage) : 0;
        robot->blood_ = new_blood;
        //判断机器人掉血后是否被击毁，如被击毁则移到击毁池，并按规定输出
        if (robot->IsDead()) {
            dead_robots_.push_back(robot);
            auto [team_id,robot_id] = robot->GetId();
            std::cout << "D" << " " << team_id << " " << robot_id << std::endl;
            //在存活池中找到目标机器人并移除
            for (auto it = live_robots_.begin(); it != live_robots_.end();) {
                if ((*it)->GetId() == std::make_tuple(team_id, robot_id)) {
                    it = live_robots_.erase(it);
                } else {
                    it++;
                }
            }
        }
    }

    //处理H指令，只针对步兵子类，增加热量
    void HandleCommandH(uint32_t team_id, uint32_t robot_id, uint32_t add_heat) {
        //在存活池中找到目标机器人
        auto robot = FindLiveRobot(team_id, robot_id);
        //没找到或其类型为工程则指令无效直接返回
        if (robot == nullptr || robot->GetType() == RobotType::kEngineer) return;
        //将机器人从父类转到步兵子类，以调用步兵子类中特有的加热量函数
        auto infantry = dynamic_pointer_cast<InfantryRobot>(robot);
        if (infantry != nullptr) {
            infantry->AddHeat(add_heat);
        }
    }

    //处理指令U，只针对步兵子类，对机器人升级
    void HandCommandU(uint32_t team_id, uint32_t robot_id, uint32_t target_level) {
        //在存活池中找到目标机器人
        auto robot = FindLiveRobot(team_id, robot_id);
        //没找到或其类型为工程则指令无效直接返回
        if (robot == nullptr || robot->GetType() == RobotType::kEngineer) return;
        //将机器人从父类转到步兵子类，以调用步兵子类中特有的升级函数
        auto infantry = dynamic_pointer_cast<InfantryRobot>(robot);
        if (infantry != nullptr) {
            infantry->Upgrade(target_level);
        }
    }
};

//主函数部分
int main() {
    //实例化管理类
    RobotManager robot_manager;
    //获取输入指令数量
    uint32_t N;
    std::cin >> N;
    //分别处理每一条输入的指令
    for (uint32_t i = 0; i < N; i++) {
        //获取指令内容
        uint32_t time;
        std::string cmd;
        uint32_t p1, p2, p3;
        std::cin >> time >> cmd >> p1 >> p2 >> p3;
        //调用管理类的时间变化处理函数
        robot_manager.HandleTimeChange(time);
        //按不同cmd类型分别调用其管理类中对应的处理函数
        if (cmd == "A") {
            robot_manager.HandleCommandA(p1, p2, static_cast<RobotType>(p3));
        } else if (cmd == "F") {
            robot_manager.HandleCommandF(p1, p2, p3);
        } else if (cmd == "H") {
            robot_manager.HandleCommandH(p1, p2, p3);
        } else if (cmd == "U") {
            robot_manager.HandCommandU(p1, p2, p3);
        }
    }
    return 0;
}
