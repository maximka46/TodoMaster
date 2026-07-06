// todo.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <json/json.h>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

const string RESET = "\033[0m";
const string RED = "\033[91m";
const string GREEN = "\033[92m";
const string YELLOW = "\033[93m";
const string BLUE = "\033[94m";
const string BOLD = "\033[1m";

string colorize(const string& text, const string& color) {
    return color + text + RESET;
}

string getHomeDir() {
    const char* home = getenv("HOME");
    if (!home) home = getenv("USERPROFILE");
    return string(home);
}

string TASKS_FILE = getHomeDir() + "/.todo_tasks.json";

struct Task {
    int id;
    string title;
    string description;
    string priority;
    string deadline;
    string status;
    string created_at;
};

vector<Task> loadTasks() {
    ifstream f(TASKS_FILE);
    vector<Task> tasks;
    if (!f) return tasks;
    Json::Value root;
    f >> root;
    for (const auto& item : root) {
        Task t;
        t.id = item["id"].asInt();
        t.title = item["title"].asString();
        t.description = item["description"].asString();
        t.priority = item["priority"].asString();
        t.deadline = item["deadline"].asString();
        t.status = item["status"].asString();
        t.created_at = item["created_at"].asString();
        tasks.push_back(t);
    }
    return tasks;
}

void saveTasks(const vector<Task>& tasks) {
    Json::Value root;
    for (const auto& t : tasks) {
        Json::Value item;
        item["id"] = t.id;
        item["title"] = t.title;
        item["description"] = t.description;
        item["priority"] = t.priority;
        item["deadline"] = t.deadline;
        item["status"] = t.status;
        item["created_at"] = t.created_at;
        root.append(item);
    }
    ofstream f(TASKS_FILE);
    f << root.toStyledString();
}

int getNextID(const vector<Task>& tasks) {
    int maxId = 0;
    for (const auto& t : tasks) {
        if (t.id > maxId) maxId = t.id;
    }
    return maxId + 1;
}

string taskColor(const Task& t) {
    if (t.status == "done") return GREEN;
    if (t.priority == "High") return RED;
    if (t.priority == "Medium") return YELLOW;
    if (t.priority == "Low") return BLUE;
    return RESET;
}

string prioritySymbol(const Task& t) {
    if (t.priority == "High") return "🔴";
    if (t.priority == "Medium") return "🟡";
    if (t.priority == "Low") return "🔵";
    return "⚪";
}

void addTask(const string& title, const string& description, const string& priority, const string& deadline) {
    vector<Task> tasks = loadTasks();
    Task t;
    t.id = getNextID(tasks);
    t.title = title;
    t.description = description;
    t.priority = priority;
    t.deadline = deadline;
    t.status = "pending";
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    t.created_at = buf;
    tasks.push_back(t);
    saveTasks(tasks);
    cout << colorize("✅ Задача #" + to_string(t.id) + " добавлена.", GREEN) << endl;
}

void listTasks(const string& filter, const string& sortBy) {
    vector<Task> tasks = loadTasks();
    if (tasks.empty()) {
        cout << colorize("Нет задач.", YELLOW) << endl;
        return;
    }
    if (!filter.empty()) {
        size_t pos = filter.find('=');
        if (pos != string::npos) {
            string key = filter.substr(0, pos);
            string val = filter.substr(pos+1);
            vector<Task> filtered;
            for (const auto& t : tasks) {
                if (key == "status" && t.status == val) filtered.push_back(t);
                else if (key == "priority" && t.priority == val) filtered.push_back(t);
            }
            tasks = filtered;
        }
    }
    if (sortBy == "priority") {
        map<string, int> order = {{"High",0},{"Medium",1},{"Low",2}};
        sort(tasks.begin(), tasks.end(), [&](const Task& a, const Task& b) {
            return order[a.priority] < order[b.priority];
        });
    } else if (sortBy == "deadline") {
        sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
            return (a.deadline.empty() ? "9999-12-31" : a.deadline) <
                   (b.deadline.empty() ? "9999-12-31" : b.deadline);
        });
    } else if (sortBy == "created") {
        sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
            return a.created_at < b.created_at;
        });
    }
    for (const auto& t : tasks) {
        string status = (t.status == "done") ? "✅" : "⬜";
        string deadline = t.deadline.empty() ? "" : " до " + t.deadline;
        cout << "[" << t.id << "] " << status << " " << t.title << " "
             << colorize(t.priority, taskColor(t)) << " " << deadline << endl;
    }
}

void doneTask(int id) {
    vector<Task> tasks = loadTasks();
    for (auto& t : tasks) {
        if (t.id == id) {
            if (t.status == "done") {
                cout << colorize("Задача #" + to_string(id) + " уже выполнена.", YELLOW) << endl;
                return;
            }
            t.status = "done";
            saveTasks(tasks);
            cout << colorize("✅ Задача #" + to_string(id) + " отмечена как выполненная.", GREEN) << endl;
            return;
        }
    }
    cout << colorize("Задача #" + to_string(id) + " не найдена.", RED) << endl;
}

void deleteTask(int id) {
    vector<Task> tasks = loadTasks();
    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
        if (it->id == id) {
            tasks.erase(it);
            saveTasks(tasks);
            cout << colorize("🗑️ Задача #" + to_string(id) + " удалена.", YELLOW) << endl;
            return;
        }
    }
    cout << colorize("Задача #" + to_string(id) + " не найдена.", RED) << endl;
}

void editTask(int id, const string& title, const string& description,
              const string& priority, const string& deadline) {
    vector<Task> tasks = loadTasks();
    for (auto& t : tasks) {
        if (t.id == id) {
            if (!title.empty()) t.title = title;
            if (!description.empty()) t.description = description;
            if (!priority.empty()) t.priority = priority;
            if (!deadline.empty()) t.deadline = deadline;
            saveTasks(tasks);
            cout << colorize("✅ Задача #" + to_string(id) + " обновлена.", GREEN) << endl;
            return;
        }
    }
    cout << colorize("Задача #" + to_string(id) + " не найдена.", RED) << endl;
}

void searchTasks(const string& query) {
    vector<Task> tasks = loadTasks();
    vector<Task> results;
    string q = query;
    transform(q.begin(), q.end(), q.begin(), ::tolower);
    for (const auto& t : tasks) {
        string title = t.title;
        string desc = t.description;
        transform(title.begin(), title.end(), title.begin(), ::tolower);
        transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
        if (title.find(q) != string::npos || desc.find(q) != string::npos) {
            results.push_back(t);
        }
    }
    if (results.empty()) {
        cout << colorize("Ничего не найдено по запросу '" + query + "'.", YELLOW) << endl;
        return;
    }
    cout << colorize("🔍 Найдено " + to_string(results.size()) + " задач:", BOLD) << endl;
    for (const auto& t : results) {
        cout << "  [" << t.id << "] " << t.title << endl;
    }
}

void stats() {
    vector<Task> tasks = loadTasks();
    int total = tasks.size();
    int done = 0;
    for (const auto& t : tasks) {
        if (t.status == "done") done++;
    }
    int pending = total - done;
    cout << colorize("📊 Статистика:", BOLD) << endl;
    cout << "  Всего задач: " << total << endl;
    if (total > 0) {
        cout << "  Выполнено: " << done << " (" << fixed << setprecision(1)
             << (double)done/total*100 << "%)" << endl;
    } else {
        cout << "  Выполнено: 0" << endl;
    }
    cout << "  Ожидают: " << pending << endl;
}

void exportTasks(const string& filename) {
    vector<Task> tasks = loadTasks();
    Json::Value root;
    for (const auto& t : tasks) {
        Json::Value item;
        item["id"] = t.id;
        item["title"] = t.title;
        item["description"] = t.description;
        item["priority"] = t.priority;
        item["deadline"] = t.deadline;
        item["status"] = t.status;
        item["created_at"] = t.created_at;
        root.append(item);
    }
    ofstream f(filename);
    f << root.toStyledString();
    cout << colorize("✅ Задачи экспортированы в " + filename, GREEN) << endl;
}

void importTasks(const string& filename) {
    ifstream f(filename);
    if (!f) {
        cout << colorize("Ошибка чтения файла.", RED) << endl;
        return;
    }
    Json::Value root;
    f >> root;
    vector<Task> tasks = loadTasks();
    for (const auto& item : root) {
        Task t;
        t.id = getNextID(tasks);
        t.title = item["title"].asString();
        t.description = item["description"].asString();
        t.priority = item["priority"].asString();
        t.deadline = item["deadline"].asString();
        t.status = item["status"].asString();
        t.created_at = item["created_at"].asString();
        tasks.push_back(t);
    }
    saveTasks(tasks);
    cout << colorize("✅ Импортировано " + to_string(root.size()) + " задач из " + filename, GREEN) << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: todo <add|list|done|delete|edit|search|stats|export|import> [args...]" << endl;
        return 1;
    }
    string cmd = argv[1];

    if (cmd == "add") {
        if (argc < 3) {
            cout << "Использование: add <title> [description] -p <priority> -d <deadline>" << endl;
            return 1;
        }
        string title = argv[2];
        string description = "";
        string priority = "Medium";
        string deadline = "";
        for (int i = 3; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "-p" && i+1 < argc) {
                priority = argv[++i];
            } else if (arg == "-d" && i+1 < argc) {
                deadline = argv[++i];
            } else if (description.empty()) {
                description = arg;
            }
        }
        addTask(title, description, priority, deadline);
    } else if (cmd == "list") {
        string filter, sortBy;
        for (int i = 2; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--filter" && i+1 < argc) {
                filter = argv[++i];
            } else if (arg == "--sort" && i+1 < argc) {
                sortBy = argv[++i];
            }
        }
        listTasks(filter, sortBy);
    } else if (cmd == "done") {
        if (argc < 3) { cout << "Использование: done <id>" << endl; return 1; }
        doneTask(stoi(argv[2]));
    } else if (cmd == "delete") {
        if (argc < 3) { cout << "Использование: delete <id>" << endl; return 1; }
        deleteTask(stoi(argv[2]));
    } else if (cmd == "edit") {
        if (argc < 3) { cout << "Использование: edit <id> [--title <title>] [--description <desc>] [--priority <p>] [--deadline <d>]" << endl; return 1; }
        int id = stoi(argv[2]);
        string title, description, priority, deadline;
        for (int i = 3; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--title" && i+1 < argc) title = argv[++i];
            else if (arg == "--description" && i+1 < argc) description = argv[++i];
            else if (arg == "--priority" && i+1 < argc) priority = argv[++i];
            else if (arg == "--deadline" && i+1 < argc) deadline = argv[++i];
        }
        editTask(id, title, description, priority, deadline);
    } else if (cmd == "search") {
        if (argc < 3) { cout << "Использование: search <query>" << endl; return 1; }
        searchTasks(argv[2]);
    } else if (cmd == "stats") {
        stats();
    } else if (cmd == "export") {
        if (argc < 3) { cout << "Использование: export <file>" << endl; return 1; }
        exportTasks(argv[2]);
    } else if (cmd == "import") {
        if (argc < 3) { cout << "Использование: import <file>" << endl; return 1; }
        importTasks(argv[2]);
    } else {
        cout << "Неизвестная команда: " << cmd << endl;
        return 1;
    }
    return 0;
}
