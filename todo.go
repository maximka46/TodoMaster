// todo.go
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"time"
)

const (
	reset  = "\033[0m"
	red    = "\033[91m"
	green  = "\033[92m"
	yellow = "\033[93m"
	blue   = "\033[94m"
	magenta= "\033[95m"
	cyan   = "\033[96m"
	bold   = "\033[1m"
)

func colorize(text, color string) string {
	return color + text + reset
}

type Task struct {
	ID          int    `json:"id"`
	Title       string `json:"title"`
	Description string `json:"description"`
	Priority    string `json:"priority"`
	Deadline    string `json:"deadline"`
	Status      string `json:"status"`
	CreatedAt   string `json:"created_at"`
}

var tasksFile = filepath.Join(os.Getenv("HOME"), ".todo_tasks.json")

func loadTasks() []Task {
	data, err := os.ReadFile(tasksFile)
	if err != nil {
		return []Task{}
	}
	var tasks []Task
	json.Unmarshal(data, &tasks)
	return tasks
}

func saveTasks(tasks []Task) {
	data, _ := json.MarshalIndent(tasks, "", "  ")
	os.WriteFile(tasksFile, data, 0644)
}

func getNextID(tasks []Task) int {
	maxID := 0
	for _, t := range tasks {
		if t.ID > maxID {
			maxID = t.ID
		}
	}
	return maxID + 1
}

func taskColor(t Task) string {
	if t.Status == "done" {
		return green
	}
	switch t.Priority {
	case "High":
		return red
	case "Medium":
		return yellow
	case "Low":
		return blue
	default:
		return reset
	}
}

func prioritySymbol(t Task) string {
	switch t.Priority {
	case "High":
		return "🔴"
	case "Medium":
		return "🟡"
	case "Low":
		return "🔵"
	default:
		return "⚪"
	}
}

func addTask(title, description, priority, deadline string) {
	tasks := loadTasks()
	task := Task{
		ID:          getNextID(tasks),
		Title:       title,
		Description: description,
		Priority:    priority,
		Deadline:    deadline,
		Status:      "pending",
		CreatedAt:   time.Now().Format(time.RFC3339),
	}
	tasks = append(tasks, task)
	saveTasks(tasks)
	fmt.Printf("%s\n", colorize(fmt.Sprintf("✅ Задача #%d добавлена.", task.ID), green))
}

func listTasks(filter, sortBy string) {
	tasks := loadTasks()
	if len(tasks) == 0 {
		fmt.Println(colorize("Нет задач.", yellow))
		return
	}
	if filter != "" {
		parts := strings.SplitN(filter, "=", 2)
		if len(parts) == 2 {
			key, val := parts[0], parts[1]
			filtered := []Task{}
			for _, t := range tasks {
				if key == "status" && t.Status == val {
					filtered = append(filtered, t)
				} else if key == "priority" && t.Priority == val {
					filtered = append(filtered, t)
				}
			}
			tasks = filtered
		}
	}
	switch sortBy {
	case "priority":
		order := map[string]int{"High": 0, "Medium": 1, "Low": 2}
		sort.Slice(tasks, func(i, j int) bool {
			return order[tasks[i].Priority] < order[tasks[j].Priority]
		})
	case "deadline":
		sort.Slice(tasks, func(i, j int) bool {
			if tasks[i].Deadline == "" {
				return false
			}
			if tasks[j].Deadline == "" {
				return true
			}
			return tasks[i].Deadline < tasks[j].Deadline
		})
	case "created":
		sort.Slice(tasks, func(i, j int) bool {
			return tasks[i].CreatedAt < tasks[j].CreatedAt
		})
	}
	for _, t := range tasks {
		status := "⬜"
		if t.Status == "done" {
			status = "✅"
		}
		deadline := ""
		if t.Deadline != "" {
			deadline = " до " + t.Deadline
		}
		fmt.Printf("[%d] %s %s %s %s\n",
			t.ID, status, t.Title, colorize(t.Priority, taskColor(t)), deadline)
	}
}

func doneTask(id int) {
	tasks := loadTasks()
	for i, t := range tasks {
		if t.ID == id {
			if t.Status == "done" {
				fmt.Println(colorize(fmt.Sprintf("Задача #%d уже выполнена.", id), yellow))
				return
			}
			tasks[i].Status = "done"
			saveTasks(tasks)
			fmt.Println(colorize(fmt.Sprintf("✅ Задача #%d отмечена как выполненная.", id), green))
			return
		}
	}
	fmt.Println(colorize(fmt.Sprintf("Задача #%d не найдена.", id), red))
}

func deleteTask(id int) {
	tasks := loadTasks()
	for i, t := range tasks {
		if t.ID == id {
			tasks = append(tasks[:i], tasks[i+1:]...)
			saveTasks(tasks)
			fmt.Println(colorize(fmt.Sprintf("🗑️ Задача #%d удалена.", id), yellow))
			return
		}
	}
	fmt.Println(colorize(fmt.Sprintf("Задача #%d не найдена.", id), red))
}

func editTask(id int, title, description, priority, deadline string) {
	tasks := loadTasks()
	for i, t := range tasks {
		if t.ID == id {
			if title != "" {
				tasks[i].Title = title
			}
			if description != "" {
				tasks[i].Description = description
			}
			if priority != "" {
				tasks[i].Priority = priority
			}
			if deadline != "" {
				tasks[i].Deadline = deadline
			}
			saveTasks(tasks)
			fmt.Println(colorize(fmt.Sprintf("✅ Задача #%d обновлена.", id), green))
			return
		}
	}
	fmt.Println(colorize(fmt.Sprintf("Задача #%d не найдена.", id), red))
}

func searchTasks(query string) {
	tasks := loadTasks()
	results := []Task{}
	for _, t := range tasks {
		if strings.Contains(strings.ToLower(t.Title), strings.ToLower(query)) ||
			strings.Contains(strings.ToLower(t.Description), strings.ToLower(query)) {
			results = append(results, t)
		}
	}
	if len(results) == 0 {
		fmt.Println(colorize(fmt.Sprintf("Ничего не найдено по запросу '%s'.", query), yellow))
		return
	}
	fmt.Printf("%s\n", colorize(fmt.Sprintf("🔍 Найдено %d задач:", len(results)), bold))
	for _, t := range results {
		fmt.Printf("  [%d] %s\n", t.ID, t.Title)
	}
}

func stats() {
	tasks := loadTasks()
	total := len(tasks)
	done := 0
	for _, t := range tasks {
		if t.Status == "done" {
			done++
		}
	}
	pending := total - done
	fmt.Println(colorize("📊 Статистика:", bold))
	fmt.Printf("  Всего задач: %d\n", total)
	if total > 0 {
		fmt.Printf("  Выполнено: %d (%.1f%%)\n", done, float64(done)/float64(total)*100)
	} else {
		fmt.Println("  Выполнено: 0")
	}
	fmt.Printf("  Ожидают: %d\n", pending)
}

func exportTasks(filename string) {
	tasks := loadTasks()
	data, _ := json.MarshalIndent(tasks, "", "  ")
	os.WriteFile(filename, data, 0644)
	fmt.Println(colorize(fmt.Sprintf("✅ Задачи экспортированы в %s", filename), green))
}

func importTasks(filename string) {
	data, err := os.ReadFile(filename)
	if err != nil {
		fmt.Println(colorize("Ошибка чтения файла: "+err.Error(), red))
		return
	}
	var imported []Task
	json.Unmarshal(data, &imported)
	tasks := loadTasks()
	for _, t := range imported {
		t.ID = getNextID(tasks)
		tasks = append(tasks, t)
	}
	saveTasks(tasks)
	fmt.Println(colorize(fmt.Sprintf("✅ Импортировано %d задач из %s", len(imported), filename), green))
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: todo <add|list|done|delete|edit|search|stats|export|import> [args...]")
		fmt.Println("  add <title> [description] -p <priority> -d <deadline>")
		fmt.Println("  list [--filter status=pending|done] [--sort priority|deadline|created]")
		fmt.Println("  done <id>")
		fmt.Println("  delete <id>")
		fmt.Println("  edit <id> --title <title> --description <desc> --priority <p> --deadline <d>")
		fmt.Println("  search <query>")
		fmt.Println("  stats")
		fmt.Println("  export <file>")
		fmt.Println("  import <file>")
		os.Exit(1)
	}
	cmd := os.Args[1]

	switch cmd {
	case "add":
		if len(os.Args) < 3 {
			fmt.Println("Использование: add <title> [description] -p <priority> -d <deadline>")
			return
		}
		title := os.Args[2]
		description := ""
		priority := "Medium"
		deadline := ""
		for i := 3; i < len(os.Args); i++ {
			if os.Args[i] == "-p" && i+1 < len(os.Args) {
				priority = os.Args[i+1]
				i++
			} else if os.Args[i] == "-d" && i+1 < len(os.Args) {
				deadline = os.Args[i+1]
				i++
			} else if description == "" {
				description = os.Args[i]
			}
		}
		addTask(title, description, priority, deadline)

	case "list":
		filter := ""
		sortBy := ""
		for i := 2; i < len(os.Args); i++ {
			if os.Args[i] == "--filter" && i+1 < len(os.Args) {
				filter = os.Args[i+1]
				i++
			} else if os.Args[i] == "--sort" && i+1 < len(os.Args) {
				sortBy = os.Args[i+1]
				i++
			}
		}
		listTasks(filter, sortBy)

	case "done":
		if len(os.Args) < 3 {
			fmt.Println("Использование: done <id>")
			return
		}
		id, _ := strconv.Atoi(os.Args[2])
		doneTask(id)

	case "delete":
		if len(os.Args) < 3 {
			fmt.Println("Использование: delete <id>")
			return
		}
		id, _ := strconv.Atoi(os.Args[2])
		deleteTask(id)

	case "edit":
		if len(os.Args) < 3 {
			fmt.Println("Использование: edit <id> [--title <title>] [--description <desc>] [--priority <p>] [--deadline <d>]")
			return
		}
		id, _ := strconv.Atoi(os.Args[2])
		title, description, priority, deadline := "", "", "", ""
		for i := 3; i < len(os.Args); i++ {
			if os.Args[i] == "--title" && i+1 < len(os.Args) {
				title = os.Args[i+1]
				i++
			} else if os.Args[i] == "--description" && i+1 < len(os.Args) {
				description = os.Args[i+1]
				i++
			} else if os.Args[i] == "--priority" && i+1 < len(os.Args) {
				priority = os.Args[i+1]
				i++
			} else if os.Args[i] == "--deadline" && i+1 < len(os.Args) {
				deadline = os.Args[i+1]
				i++
			}
		}
		editTask(id, title, description, priority, deadline)

	case "search":
		if len(os.Args) < 3 {
			fmt.Println("Использование: search <query>")
			return
		}
		searchTasks(os.Args[2])

	case "stats":
		stats()

	case "export":
		if len(os.Args) < 3 {
			fmt.Println("Использование: export <file>")
			return
		}
		exportTasks(os.Args[2])

	case "import":
		if len(os.Args) < 3 {
			fmt.Println("Использование: import <file>")
			return
		}
		importTasks(os.Args[2])

	default:
		fmt.Println("Неизвестная команда:", cmd)
	}
}
