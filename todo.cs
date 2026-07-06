// todo.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

class Task
{
    public int Id { get; set; }
    public string Title { get; set; }
    public string Description { get; set; }
    public string Priority { get; set; }
    public string Deadline { get; set; }
    public string Status { get; set; }
    public string CreatedAt { get; set; }
}

class Todo
{
    static string Colorize(string text, string color)
    {
        string col = color switch
        {
            "red" => "\x1b[91m",
            "green" => "\x1b[92m",
            "yellow" => "\x1b[93m",
            "blue" => "\x1b[94m",
            "bold" => "\x1b[1m",
            _ => "\x1b[0m"
        };
        return col + text + "\x1b[0m";
    }

    static string TasksFile = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".todo_tasks.json");

    static List<Task> LoadTasks()
    {
        if (!File.Exists(TasksFile)) return new List<Task>();
        string json = File.ReadAllText(TasksFile);
        return JsonSerializer.Deserialize<List<Task>>(json) ?? new List<Task>();
    }

    static void SaveTasks(List<Task> tasks)
    {
        string json = JsonSerializer.Serialize(tasks, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(TasksFile, json);
    }

    static int GetNextID(List<Task> tasks) => tasks.Count > 0 ? tasks.Max(t => t.Id) + 1 : 1;

    static string TaskColor(Task t)
    {
        if (t.Status == "done") return "green";
        return t.Priority switch
        {
            "High" => "red",
            "Medium" => "yellow",
            "Low" => "blue",
            _ => "reset"
        };
    }

    static string PrioritySymbol(Task t)
    {
        return t.Priority switch
        {
            "High" => "🔴",
            "Medium" => "🟡",
            "Low" => "🔵",
            _ => "⚪"
        };
    }

    static void AddTask(string title, string description, string priority, string deadline)
    {
        var tasks = LoadTasks();
        var task = new Task
        {
            Id = GetNextID(tasks),
            Title = title,
            Description = description ?? "",
            Priority = priority ?? "Medium",
            Deadline = deadline ?? "",
            Status = "pending",
            CreatedAt = DateTime.Now.ToString("o")
        };
        tasks.Add(task);
        SaveTasks(tasks);
        Console.WriteLine(Colorize($"✅ Задача #{task.Id} добавлена.", "green"));
    }

    static void ListTasks(string filter, string sortBy)
    {
        var tasks = LoadTasks();
        if (tasks.Count == 0)
        {
            Console.WriteLine(Colorize("Нет задач.", "yellow"));
            return;
        }
        if (!string.IsNullOrEmpty(filter))
        {
            var parts = filter.Split('=');
            if (parts.Length == 2)
            {
                string key = parts[0], val = parts[1];
                tasks = tasks.Where(t =>
                    (key == "status" && t.Status == val) ||
                    (key == "priority" && t.Priority == val)
                ).ToList();
            }
        }
        if (sortBy == "priority")
        {
            var order = new Dictionary<string, int> { { "High", 0 }, { "Medium", 1 }, { "Low", 2 } };
            tasks = tasks.OrderBy(t => order[t.Priority]).ToList();
        }
        else if (sortBy == "deadline")
        {
            tasks = tasks.OrderBy(t => t.Deadline ?? "9999-12-31").ToList();
        }
        else if (sortBy == "created")
        {
            tasks = tasks.OrderBy(t => t.CreatedAt).ToList();
        }
        foreach (var t in tasks)
        {
            string status = t.Status == "done" ? "✅" : "⬜";
            string deadline = string.IsNullOrEmpty(t.Deadline) ? "" : $" до {t.Deadline}";
            Console.WriteLine($"[{t.Id}] {status} {t.Title} {Colorize(t.Priority, TaskColor(t))} {deadline}");
        }
    }

    static void DoneTask(int id)
    {
        var tasks = LoadTasks();
        var task = tasks.FirstOrDefault(t => t.Id == id);
        if (task == null)
        {
            Console.WriteLine(Colorize($"Задача #{id} не найдена.", "red"));
            return;
        }
        if (task.Status == "done")
        {
            Console.WriteLine(Colorize($"Задача #{id} уже выполнена.", "yellow"));
            return;
        }
        task.Status = "done";
        SaveTasks(tasks);
        Console.WriteLine(Colorize($"✅ Задача #{id} отмечена как выполненная.", "green"));
    }

    static void DeleteTask(int id)
    {
        var tasks = LoadTasks();
        var idx = tasks.FindIndex(t => t.Id == id);
        if (idx == -1)
        {
            Console.WriteLine(Colorize($"Задача #{id} не найдена.", "red"));
            return;
        }
        tasks.RemoveAt(idx);
        SaveTasks(tasks);
        Console.WriteLine(Colorize($"🗑️ Задача #{id} удалена.", "yellow"));
    }

    static void EditTask(int id, string title, string description, string priority, string deadline)
    {
        var tasks = LoadTasks();
        var task = tasks.FirstOrDefault(t => t.Id == id);
        if (task == null)
        {
            Console.WriteLine(Colorize($"Задача #{id} не найдена.", "red"));
            return;
        }
        if (!string.IsNullOrEmpty(title)) task.Title = title;
        if (!string.IsNullOrEmpty(description)) task.Description = description;
        if (!string.IsNullOrEmpty(priority)) task.Priority = priority;
        if (!string.IsNullOrEmpty(deadline)) task.Deadline = deadline;
        SaveTasks(tasks);
        Console.WriteLine(Colorize($"✅ Задача #{id} обновлена.", "green"));
    }

    static void SearchTasks(string query)
    {
        var tasks = LoadTasks();
        var results = tasks.Where(t =>
            t.Title.Contains(query, StringComparison.OrdinalIgnoreCase) ||
            t.Description.Contains(query, StringComparison.OrdinalIgnoreCase)
        ).ToList();
        if (results.Count == 0)
        {
            Console.WriteLine(Colorize($"Ничего не найдено по запросу '{query}'.", "yellow"));
            return;
        }
        Console.WriteLine(Colorize($"🔍 Найдено {results.Count} задач:", "bold"));
        foreach (var t in results) Console.WriteLine($"  [{t.Id}] {t.Title}");
    }

    static void Stats()
    {
        var tasks = LoadTasks();
        int total = tasks.Count;
        int done = tasks.Count(t => t.Status == "done");
        int pending = total - done;
        Console.WriteLine(Colorize("📊 Статистика:", "bold"));
        Console.WriteLine($"  Всего задач: {total}");
        if (total > 0)
            Console.WriteLine($"  Выполнено: {done} ({(double)done/total*100:F1}%)");
        else
            Console.WriteLine("  Выполнено: 0");
        Console.WriteLine($"  Ожидают: {pending}");
    }

    static void ExportTasks(string filename)
    {
        var tasks = LoadTasks();
        string json = JsonSerializer.Serialize(tasks, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(filename, json);
        Console.WriteLine(Colorize($"✅ Задачи экспортированы в {filename}", "green"));
    }

    static void ImportTasks(string filename)
    {
        if (!File.Exists(filename))
        {
            Console.WriteLine(Colorize("Файл не найден.", "red"));
            return;
        }
        string json = File.ReadAllText(filename);
        var imported = JsonSerializer.Deserialize<List<Task>>(json) ?? new List<Task>();
        var tasks = LoadTasks();
        foreach (var t in imported)
        {
            t.Id = GetNextID(tasks);
            tasks.Add(t);
        }
        SaveTasks(tasks);
        Console.WriteLine(Colorize($"✅ Импортировано {imported.Count} задач из {filename}", "green"));
    }

    static void Main(string[] args)
    {
        if (args.Length == 0)
        {
            Console.WriteLine("Usage: todo <add|list|done|delete|edit|search|stats|export|import> [args...]");
            return;
        }
        string cmd = args[0];

        switch (cmd)
        {
            case "add":
                if (args.Length < 2) { Console.WriteLine("Использование: add <title> [description] -p <priority> -d <deadline>"); return; }
                string title = args[1], description = "", priority = "Medium", deadline = "";
                for (int i = 2; i < args.Length; i++)
                {
                    if (args[i] == "-p" && i+1 < args.Length) priority = args[++i];
                    else if (args[i] == "-d" && i+1 < args.Length) deadline = args[++i];
                    else if (string.IsNullOrEmpty(description)) description = args[i];
                }
                AddTask(title, description, priority, deadline);
                break;
            case "list":
                string filter = "", sortBy = "";
                for (int i = 1; i < args.Length; i++)
                {
                    if (args[i] == "--filter" && i+1 < args.Length) filter = args[++i];
                    else if (args[i] == "--sort" && i+1 < args.Length) sortBy = args[++i];
                }
                ListTasks(filter, sortBy);
                break;
            case "done":
                if (args.Length < 2) { Console.WriteLine("Использование: done <id>"); return; }
                DoneTask(int.Parse(args[1]));
                break;
            case "delete":
                if (args.Length < 2) { Console.WriteLine("Использование: delete <id>"); return; }
                DeleteTask(int.Parse(args[1]));
                break;
            case "edit":
                if (args.Length < 3) { Console.WriteLine("Использование: edit <id> [--title <title>] [--description <desc>] [--priority <p>] [--deadline <d>]"); return; }
                int id = int.Parse(args[1]);
                string ttl = "", desc = "", pri = "", ddl = "";
                for (int i = 2; i < args.Length; i++)
                {
                    if (args[i] == "--title" && i+1 < args.Length) ttl = args[++i];
                    else if (args[i] == "--description" && i+1 < args.Length) desc = args[++i];
                    else if (args[i] == "--priority" && i+1 < args.Length) pri = args[++i];
                    else if (args[i] == "--deadline" && i+1 < args.Length) ddl = args[++i];
                }
                EditTask(id, ttl, desc, pri, ddl);
                break;
            case "search":
                if (args.Length < 2) { Console.WriteLine("Использование: search <query>"); return; }
                SearchTasks(args[1]);
                break;
            case "stats":
                Stats();
                break;
            case "export":
                if (args.Length < 2) { Console.WriteLine("Использование: export <file>"); return; }
                ExportTasks(args[1]);
                break;
            case "import":
                if (args.Length < 2) { Console.WriteLine("Использование: import <file>"); return; }
                ImportTasks(args[1]);
                break;
            default:
                Console.WriteLine($"Неизвестная команда: {cmd}");
                break;
        }
    }
}
