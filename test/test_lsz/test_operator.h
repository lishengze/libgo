#include "global_def.h"
#include <functional>


typedef std::function<void()> GO_TASK;

#define GO CoRoutine(__FILE__, __LINE__)-

class CoRoutine {
    public:
        CoRoutine(const char * file_name, int lineno): file_name_{file_name}, lineno_{lineno}
        {
            cout << "file_name: " << file_name_ << ", lineno_: " << lineno << endl;

            InitListen();
        }

        ~CoRoutine() {
            if (listen_thread_.joinable()) {
                listen_thread_.join();
            }
        }

        template<typename Function>
        void operator-(const Function& f) { 

            cout << "operator-(const Function& f) " << endl;
            AddTask(f);
        }

        void AddTask(const GO_TASK& task) {
            cout << "AddTask " << endl;
            tasks_.push_back(task);
        }

        void InitListen() {
            cout << "InitListen " << endl;
            listen_thread_ = std::thread(&CoRoutine::ListenMain, this);
        }

        void ListenMain() {
            cout << "ListenMain " << endl;

            while(true) {

                std::this_thread::sleep_for(std::chrono::seconds(3));

                for (auto& task:tasks_) {
                    task();
                }
            }
        }

    string          file_name_;
    int             lineno_;
    vector<GO_TASK> tasks_;

    thread          listen_thread_;

};

void func1() {
    printf("This is Func1\n");
}

void TestCoRoutine() {
    GO func1;
}