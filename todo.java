// todo.java
import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.util.*;
import com.google.gson.*; // install gson

public class todo {
    private static final String RESET = "\u001B[0m";
    private static final String RED = "\u001B[91m";
    private static final String GREEN = "\u001B[92m";
    private static final String YELLOW = "\u001B[93m";
    private static final String BLUE = "\u001B[94m";
    private static final String BOLD = "\u001B[1m";

    private static String colorize(String text, String color) {
        return color + text + RESET;
    }

    private static String tasksFile = System.getProperty("user.home") + "/.todo_tasks.json";

    static class Task {
        int id;
        String title;
        String description;
        String priority;
        String deadline;
        String status;
        String created_at;
    }

    private static List<Task> loadTasks() throws IOException {
        Path path = Paths.get(tasksFile);
        if (!Files.exists(path)) return new ArrayList<>();
        String json = new String(Files.readAllBytes(path));
        Gson gson = new Gson();
        Task[] arr = gson.fromJson(json, Task[].class);
        return new ArrayList<>(Arrays.asList(arr));
    }

    private static void saveTasks(List<Task> tasks) throws IOException {
        Gson gson = new GsonBuilder().setPrettyPrinting().create();
        String json = gson.toJson(tasks);
        Files.write(Paths.get(tasksFile), json.getBytes());
    }

    private static int getNextID(List<Task> tasks) {
        int max = 0;
        for (Task t : tasks) {
            if (t.id > max) max = t.id;
        }
        return max + 1;
    }

    private static String taskColor(Task t) {
        if (t.status.equals("done")) return GREEN;
        switch (t.priority) {
            case "High": return RED;
            case "Medium": return YELLOW;
            case "Low": return BLUE;
            default: return RESET;
        }
    }

    private static String prioritySymbol(Task t) {
        switch (t.priority) {
            case "High": return "🔴";
            case "Medium": return "🟡";
            case "Low": return "🔵";
            default: return "⚪";
        }
    }

    private static void addTask(String title, String description, String priority, String deadline) throws IOException {
        List<Task> tasks = loadTasks();
        Task t = new Task();
        t.id = getNextID(tasks);
        t.title = title;
        t.description = description == null ? "" : description;
        t.priority = priority == null ? "Medium" : priority;
        t.deadline = deadline == null ? "" : deadline;
        t.status = "pending";
        t.created_at = Instant.now().toString();
        tasks.add(t);
        saveTasks(tasks);
        System.out.println(colorize("✅ Задача #" + t.id + " добавлена.", GREEN));
    }

    private static void listTasks(String filter, String sortBy) throws IOException {
        List<Task> tasks = loadTasks();
        if (tasks.isEmpty()) {
            System.out.println(colorize("Нет задач.", YELLOW));
            return;
        }
        if (filter != null) {
            String[] parts = filter.split("=");
            if (parts.length == 2) {
                String key = parts[0], val = parts[1];
                List<Task> filtered = new ArrayList<>();
                for (Task t : tasks) {
                    if (key.equals("status") && t.status.equals(val)) filtered.add(t);
                    else if (key.equals("priority") && t.priority.equals(val)) filtered.add(t);
                }
                tasks = filtered;
            }
        }
        if (sortBy != null) {
            switch (sortBy) {
                case "priority":
                    Map<String, Integer> order = new HashMap<>();
                    order.put("High", 0);
                    order.put("Medium", 1);
                    order.put("Low", 2);
                    tasks.sort((a, b) -> order.get(a.priority) - order.get(b.priority));
                    break;
                case "deadline":
                    tasks.sort((a, b) -> {
                        String da = a.deadline.isEmpty() ? "9999-12-31" : a.deadline;
                        String db = b.deadline.isEmpty() ? "9999-12-31" : b.deadline;
                        return da.compareTo(db);
                    });
                    break;
                case "created":
                    tasks.sort((a, b) -> a.created_at.compareTo(b.created_at));
                    break;
            }
        }
        for (Task t : tasks) {
            String status = t.status.equals("done") ? "✅" : "⬜";
            String deadline = t.deadline.isEmpty() ? "" : " до " + t.deadline;
            System.out.printf("[%d] %s %s %s %s\n",
                t.id, status, t.title, colorize(t.priority, taskColor(t)), deadline);
        }
    }

    private static void doneTask(int id) throws IOException {
        List<Task> tasks = loadTasks();
        Task found = null;
        for (Task t : tasks) {
            if (t.id == id) { found = t; break; }
        }
        if (found == null) {
            System.out.println(colorize("Задача #" + id + " не найдена.", RED));
            return;
        }
        if (found.status.equals("done")) {
            System.out.println(colorize("Задача #" + id + " уже выполнена.", YELLOW));
            return;
        }
        found.status = "done";
        saveTasks(tasks);
        System.out.println(colorize("✅ Задача #" + id + " отмечена как выполненная.", GREEN));
    }

    private static void deleteTask(int id) throws IOException {
        List<Task> tasks = loadTasks();
        Iterator<Task> it = tasks.iterator();
        while (it.hasNext()) {
            Task t = it.next();
            if (t.id == id) {
                it.remove();
                saveTasks(tasks);
                System.out.println(colorize("🗑️ Задача #" + id + " удалена.", YELLOW));
                return;
            }
        }
        System.out.println(colorize("Задача #" + id + " не найдена.", RED));
    }

    private static void editTask(int id, String title, String description, String priority, String deadline) throws IOException {
        List<Task> tasks = loadTasks();
        Task found = null;
        for (Task t : tasks) {
            if (t.id == id) { found = t; break; }
        }
        if (found == null) {
            System.out.println(colorize("Задача #" + id + " не найдена.", RED));
            return;
        }
        if (title != null) found.title = title;
        if (description != null) found.description = description;
        if (priority != null) found.priority = priority;
        if (deadline != null) found.deadline = deadline;
        saveTasks(tasks);
        System.out.println(colorize("✅ Задача #" + id + " обновлена.", GREEN));
    }

    private static void searchTasks(String query) throws IOException {
        List<Task> tasks = loadTasks();
        List<Task> results = new ArrayList<>();
        String q = query.toLowerCase();
        for (Task t : tasks) {
            if (t.title.toLowerCase().contains(q) || t.description.toLowerCase().contains(q)) {
                results.add(t);
            }
        }
        if (results.isEmpty()) {
            System.out.println(colorize("Ничего не найдено по запросу '" + query + "'.", YELLOW));
            return;
        }
        System.out.println(colorize("🔍 Найдено " + results.size() + " задач:", BOLD));
        for (Task t : results) {
            System.out.println("  [" + t.id + "] " + t.title);
        }
    }

    private static void stats() throws IOException {
        List<Task> tasks = loadTasks();
        int total = tasks.size();
        int done = 0;
        for (Task t : tasks) {
            if (t.status.equals("done")) done++;
        }
        int pending = total - done;
        System.out.println(colorize("📊 Статистика:", BOLD));
        System.out.println("  Всего задач: " + total);
        if (total > 0) {
            System.out.printf("  Выполнено: %d (%.1f%%)\n", done, (double)done / total * 100);
        } else {
            System.out.println("  Выполнено: 0");
        }
        System.out.println("  Ожидают: " + pending);
    }

    private static void exportTasks(String filename) throws IOException {
        List<Task> tasks = loadTasks();
        Gson gson = new GsonBuilder().setPrettyPrinting().create();
        String json = gson.toJson(tasks);
        Files.write(Paths.get(filename), json.getBytes());
        System.out.println(colorize("✅ Задачи экспортированы в " + filename, GREEN));
    }

    private static void importTasks(String filename) throws IOException {
        if (!Files.exists(Paths.get(filename))) {
            System.out.println(colorize("Файл не найден.", RED));
            return;
        }
        String json = new String(Files.readAllBytes(Paths.get(filename)));
        Gson gson = new Gson();
        Task[] arr = gson.fromJson(json, Task[].class);
        List<Task> tasks = loadTasks();
        for (Task t : arr) {
            t.id = getNextID(tasks);
            tasks.add(t);
        }
        saveTasks(tasks);
        System.out.println(colorize("✅ Импортировано " + arr.length + " задач из " + filename, GREEN));
    }

    public static void main(String[] args) throws IOException {
        if (args.length == 0) {
            System.out.println("Usage: java todo <add|list|done|delete|edit|search|stats|export|import> [args...]");
            System.out.println("  add <title> [description] -p <priority> -d <deadline>");
            System.out.println("  list [--filter status=pending|done] [--sort priority|deadline|created]");
            System.out.println("  done <id>");
            System.out.println("  delete <id>");
            System.out.println("  edit <id> --title <title> --description <desc> --priority <p> --deadline <d>");
            System.out.println("  search <query>");
            System.out.println("  stats");
            System.out.println("  export <file>");
            System.out.println("  import <file>");
            return;
        }
        String cmd = args[0];

        switch (cmd) {
            case "add": {
                if (args.length < 2) {
                    System.out.println("Использование: add <title> [description] -p <priority> -d <deadline>");
                    return;
                }
                String title = args[1];
                String description = null, priority = "Medium", deadline = null;
                for (int i = 2; i < args.length; i++) {
                    if (args[i].equals("-p") && i+1 < args.length) {
                        priority = args[++i];
                    } else if (args[i].equals("-d") && i+1 < args.length) {
                        deadline = args[++i];
                    } else if (description == null) {
                        description = args[i];
                    }
                }
                addTask(title, description, priority, deadline);
                break;
            }
            case "list": {
                String filter = null, sortBy = null;
                for (int i = 1; i < args.length; i++) {
                    if (args[i].equals("--filter") && i+1 < args.length) {
                        filter = args[++i];
                    } else if (args[i].equals("--sort") && i+1 < args.length) {
                        sortBy = args[++i];
                    }
                }
                listTasks(filter, sortBy);
                break;
            }
            case "done": {
                if (args.length < 2) {
                    System.out.println("Использование: done <id>");
                    return;
                }
                doneTask(Integer.parseInt(args[1]));
                break;
            }
            case "delete": {
                if (args.length < 2) {
                    System.out.println("Использование: delete <id>");
                    return;
                }
                deleteTask(Integer.parseInt(args[1]));
                break;
            }
            case "edit": {
                if (args.length < 3) {
                    System.out.println("Использование: edit <id> --title <title> --description <desc> --priority <p> --deadline <d>");
                    return;
                }
                int id = Integer.parseInt(args[1]);
                String title = null, description = null, priority = null, deadline = null;
                for (int i = 2; i < args.length; i++) {
                    if (args[i].equals("--title") && i+1 < args.length) {
                        title = args[++i];
                    } else if (args[i].equals("--description") && i+1 < args.length) {
                        description = args[++i];
                    } else if (args[i].equals("--priority") && i+1 < args.length) {
                        priority = args[++i];
                    } else if (args[i].equals("--deadline") && i+1 < args.length) {
                        deadline = args[++i];
                    }
                }
                editTask(id, title, description, priority, deadline);
                break;
            }
            case "search": {
                if (args.length < 2) {
                    System.out.println("Использование: search <query>");
                    return;
                }
                searchTasks(args[1]);
                break;
            }
            case "stats":
                stats();
                break;
            case "export": {
                if (args.length < 2) {
                    System.out.println("Использование: export <file>");
                    return;
                }
                exportTasks(args[1]);
                break;
            }
            case "import": {
                if (args.length < 2) {
                    System.out.println("Использование: import <file>");
                    return;
                }
                importTasks(args[1]);
                break;
            }
            default:
                System.out.println("Неизвестная команда: " + cmd);
        }
    }
}
