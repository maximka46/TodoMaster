// todo.js
#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');
const os = require('os');

const COLORS = {
    reset: '\x1b[0m',
    red: '\x1b[91m',
    green: '\x1b[92m',
    yellow: '\x1b[93m',
    blue: '\x1b[94m',
    magenta: '\x1b[95m',
    cyan: '\x1b[96m',
    bold: '\x1b[1m'
};

function colorize(text, color) {
    return COLORS[color] + text + COLORS.reset;
}

const TASKS_FILE = path.join(os.homedir(), '.todo_tasks.json');

function loadTasks() {
    try {
        return JSON.parse(fs.readFileSync(TASKS_FILE, 'utf8'));
    } catch {
        return [];
    }
}

function saveTasks(tasks) {
    fs.writeFileSync(TASKS_FILE, JSON.stringify(tasks, null, 2));
}

function getNextID(tasks) {
    let max = 0;
    for (const t of tasks) {
        if (t.id > max) max = t.id;
    }
    return max + 1;
}

function taskColor(task) {
    if (task.status === 'done') return 'green';
    switch (task.priority) {
        case 'High': return 'red';
        case 'Medium': return 'yellow';
        case 'Low': return 'blue';
        default: return 'reset';
    }
}

function prioritySymbol(task) {
    switch (task.priority) {
        case 'High': return '🔴';
        case 'Medium': return '🟡';
        case 'Low': return '🔵';
        default: return '⚪';
    }
}

function addTask(title, description, priority, deadline) {
    const tasks = loadTasks();
    const task = {
        id: getNextID(tasks),
        title,
        description: description || '',
        priority: priority || 'Medium',
        deadline: deadline || '',
        status: 'pending',
        created_at: new Date().toISOString()
    };
    tasks.push(task);
    saveTasks(tasks);
    console.log(colorize(`✅ Задача #${task.id} добавлена.`, 'green'));
}

function listTasks(filter, sortBy) {
    let tasks = loadTasks();
    if (tasks.length === 0) {
        console.log(colorize('Нет задач.', 'yellow'));
        return;
    }
    if (filter) {
        const [key, val] = filter.split('=');
        tasks = tasks.filter(t => t[key] === val);
    }
    if (sortBy === 'priority') {
        const order = { High: 0, Medium: 1, Low: 2 };
        tasks.sort((a, b) => order[a.priority] - order[b.priority]);
    } else if (sortBy === 'deadline') {
        tasks.sort((a, b) => (a.deadline || '9999-12-31').localeCompare(b.deadline || '9999-12-31'));
    } else if (sortBy === 'created') {
        tasks.sort((a, b) => a.created_at.localeCompare(b.created_at));
    }
    for (const t of tasks) {
        const status = t.status === 'done' ? '✅' : '⬜';
        const deadline = t.deadline ? ` до ${t.deadline}` : '';
        const priorityColored = colorize(t.priority, taskColor(t));
        console.log(`[${t.id}] ${status} ${t.title} ${priorityColored} ${deadline}`);
    }
}

function doneTask(id) {
    const tasks = loadTasks();
    const task = tasks.find(t => t.id === id);
    if (!task) {
        console.log(colorize(`Задача #${id} не найдена.`, 'red'));
        return;
    }
    if (task.status === 'done') {
        console.log(colorize(`Задача #${id} уже выполнена.`, 'yellow'));
        return;
    }
    task.status = 'done';
    saveTasks(tasks);
    console.log(colorize(`✅ Задача #${id} отмечена как выполненная.`, 'green'));
}

function deleteTask(id) {
    let tasks = loadTasks();
    const idx = tasks.findIndex(t => t.id === id);
    if (idx === -1) {
        console.log(colorize(`Задача #${id} не найдена.`, 'red'));
        return;
    }
    tasks.splice(idx, 1);
    saveTasks(tasks);
    console.log(colorize(`🗑️ Задача #${id} удалена.`, 'yellow'));
}

function editTask(id, title, description, priority, deadline) {
    const tasks = loadTasks();
    const task = tasks.find(t => t.id === id);
    if (!task) {
        console.log(colorize(`Задача #${id} не найдена.`, 'red'));
        return;
    }
    if (title) task.title = title;
    if (description) task.description = description;
    if (priority) task.priority = priority;
    if (deadline) task.deadline = deadline;
    saveTasks(tasks);
    console.log(colorize(`✅ Задача #${id} обновлена.`, 'green'));
}

function searchTasks(query) {
    const tasks = loadTasks();
    const results = tasks.filter(t =>
        t.title.toLowerCase().includes(query.toLowerCase()) ||
        t.description.toLowerCase().includes(query.toLowerCase())
    );
    if (results.length === 0) {
        console.log(colorize(`Ничего не найдено по запросу '${query}'.`, 'yellow'));
        return;
    }
    console.log(colorize(`🔍 Найдено ${results.length} задач:`, 'bold'));
    for (const t of results) {
        console.log(`  [${t.id}] ${t.title}`);
    }
}

function stats() {
    const tasks = loadTasks();
    const total = tasks.length;
    const done = tasks.filter(t => t.status === 'done').length;
    const pending = total - done;
    console.log(colorize('📊 Статистика:', 'bold'));
    console.log(`  Всего задач: ${total}`);
    if (total > 0) {
        console.log(`  Выполнено: ${done} (${(done/total*100).toFixed(1)}%)`);
    } else {
        console.log('  Выполнено: 0');
    }
    console.log(`  Ожидают: ${pending}`);
}

function exportTasks(filename) {
    const tasks = loadTasks();
    fs.writeFileSync(filename, JSON.stringify(tasks, null, 2));
    console.log(colorize(`✅ Задачи экспортированы в ${filename}`, 'green'));
}

function importTasks(filename) {
    const data = JSON.parse(fs.readFileSync(filename, 'utf8'));
    const tasks = loadTasks();
    for (const t of data) {
        t.id = getNextID(tasks);
        tasks.push(t);
    }
    saveTasks(tasks);
    console.log(colorize(`✅ Импортировано ${data.length} задач из ${filename}`, 'green'));
}

function main() {
    const args = process.argv.slice(2);
    if (args.length === 0) {
        console.log(`Usage: node todo.js <add|list|done|delete|edit|search|stats|export|import> [args...]`);
        console.log(`  add <title> [description] -p <priority> -d <deadline>`);
        console.log(`  list [--filter status=pending|done] [--sort priority|deadline|created]`);
        console.log(`  done <id>`);
        console.log(`  delete <id>`);
        console.log(`  edit <id> --title <title> --description <desc> --priority <p> --deadline <d>`);
        console.log(`  search <query>`);
        console.log(`  stats`);
        console.log(`  export <file>`);
        console.log(`  import <file>`);
        process.exit(1);
    }
    const cmd = args[0];

    switch (cmd) {
        case 'add': {
            if (args.length < 2) {
                console.log('Использование: add <title> [description] -p <priority> -d <deadline>');
                return;
            }
            let title = args[1];
            let description = '';
            let priority = 'Medium';
            let deadline = '';
            for (let i = 2; i < args.length; i++) {
                if (args[i] === '-p' && i+1 < args.length) {
                    priority = args[++i];
                } else if (args[i] === '-d' && i+1 < args.length) {
                    deadline = args[++i];
                } else if (!description) {
                    description = args[i];
                }
            }
            addTask(title, description, priority, deadline);
            break;
        }
        case 'list': {
            let filter = '';
            let sortBy = '';
            for (let i = 1; i < args.length; i++) {
                if (args[i] === '--filter' && i+1 < args.length) {
                    filter = args[++i];
                } else if (args[i] === '--sort' && i+1 < args.length) {
                    sortBy = args[++i];
                }
            }
            listTasks(filter, sortBy);
            break;
        }
        case 'done': {
            if (args.length < 2) {
                console.log('Использование: done <id>');
                return;
            }
            doneTask(parseInt(args[1]));
            break;
        }
        case 'delete': {
            if (args.length < 2) {
                console.log('Использование: delete <id>');
                return;
            }
            deleteTask(parseInt(args[1]));
            break;
        }
        case 'edit': {
            if (args.length < 3) {
                console.log('Использование: edit <id> [--title <title>] [--description <desc>] [--priority <p>] [--deadline <d>]');
                return;
            }
            const id = parseInt(args[1]);
            let title = '', description = '', priority = '', deadline = '';
            for (let i = 2; i < args.length; i++) {
                if (args[i] === '--title' && i+1 < args.length) {
                    title = args[++i];
                } else if (args[i] === '--description' && i+1 < args.length) {
                    description = args[++i];
                } else if (args[i] === '--priority' && i+1 < args.length) {
                    priority = args[++i];
                } else if (args[i] === '--deadline' && i+1 < args.length) {
                    deadline = args[++i];
                }
            }
            editTask(id, title, description, priority, deadline);
            break;
        }
        case 'search': {
            if (args.length < 2) {
                console.log('Использование: search <query>');
                return;
            }
            searchTasks(args[1]);
            break;
        }
        case 'stats':
            stats();
            break;
        case 'export': {
            if (args.length < 2) {
                console.log('Использование: export <file>');
                return;
            }
            exportTasks(args[1]);
            break;
        }
        case 'import': {
            if (args.length < 2) {
                console.log('Использование: import <file>');
                return;
            }
            importTasks(args[1]);
            break;
        }
        default:
            console.log('Неизвестная команда:', cmd);
    }
}

main();
