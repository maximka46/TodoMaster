# todo.py
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import json
import argparse
from datetime import datetime
from pathlib import Path

# ANSI-цвета
COLORS = {
    'reset': '\033[0m',
    'red': '\033[91m',
    'green': '\033[92m',
    'yellow': '\033[93m',
    'blue': '\033[94m',
    'magenta': '\033[95m',
    'cyan': '\033[96m',
    'bold': '\033[1m'
}

def colorize(text, color):
    return f"{COLORS.get(color, '')}{text}{COLORS['reset']}"

# Путь к файлу задач
HOME = os.path.expanduser('~')
TASKS_FILE = os.path.join(HOME, '.todo_tasks.json')

class Task:
    def __init__(self, title, description='', priority='Medium', deadline=None):
        self.id = None
        self.title = title
        self.description = description
        self.priority = priority  # High, Medium, Low
        self.deadline = deadline  # YYYY-MM-DD
        self.status = 'pending'   # pending, done
        self.created_at = datetime.now().isoformat()

    def to_dict(self):
        return {
            'id': self.id,
            'title': self.title,
            'description': self.description,
            'priority': self.priority,
            'deadline': self.deadline,
            'status': self.status,
            'created_at': self.created_at
        }

    @staticmethod
    def from_dict(data):
        task = Task(data['title'], data.get('description', ''), data['priority'], data.get('deadline'))
        task.id = data['id']
        task.status = data.get('status', 'pending')
        task.created_at = data.get('created_at', datetime.now().isoformat())
        return task

    def get_color(self):
        if self.status == 'done':
            return 'green'
        return {'High': 'red', 'Medium': 'yellow', 'Low': 'blue'}.get(self.priority, 'reset')

    def get_priority_symbol(self):
        return {'High': '🔴', 'Medium': '🟡', 'Low': '🔵'}.get(self.priority, '⚪')

    def __str__(self):
        status = '✅' if self.status == 'done' else '⬜'
        deadline = f" до {self.deadline}" if self.deadline else ""
        return f"[{self.id}] {status} {self.title} {self.get_priority_symbol()} {deadline}"

def load_tasks():
    if os.path.exists(TASKS_FILE):
        with open(TASKS_FILE, 'r') as f:
            data = json.load(f)
            return [Task.from_dict(t) for t in data]
    return []

def save_tasks(tasks):
    with open(TASKS_FILE, 'w') as f:
        json.dump([t.to_dict() for t in tasks], f, indent=2)

def get_next_id(tasks):
    return max([t.id for t in tasks] + [0]) + 1

def add_task(args):
    tasks = load_tasks()
    task = Task(args.title, args.description, args.priority, args.deadline)
    task.id = get_next_id(tasks)
    tasks.append(task)
    save_tasks(tasks)
    print(colorize(f"✅ Задача #{task.id} добавлена.", 'green'))

def list_tasks(args):
    tasks = load_tasks()
    if not tasks:
        print(colorize("Нет задач.", 'yellow'))
        return

    # Фильтрация
    if args.filter:
        key, val = args.filter.split('=')
        if key == 'status':
            tasks = [t for t in tasks if t.status == val]
        elif key == 'priority':
            tasks = [t for t in tasks if t.priority == val]

    # Сортировка
    if args.sort == 'priority':
        order = {'High': 0, 'Medium': 1, 'Low': 2}
        tasks.sort(key=lambda t: order.get(t.priority, 3))
    elif args.sort == 'deadline':
        tasks.sort(key=lambda t: t.deadline or '9999-12-31')
    elif args.sort == 'created':
        tasks.sort(key=lambda t: t.created_at)

    for task in tasks:
        color = task.get_color()
        status = '✅' if task.status == 'done' else '⬜'
        priority_col = colorize(task.priority, color)
        deadline_str = f" до {task.deadline}" if task.deadline else ""
        print(f"[{task.id}] {status} {task.title} {priority_col} {deadline_str}")

def done_task(args):
    tasks = load_tasks()
    task = next((t for t in tasks if t.id == args.id), None)
    if not task:
        print(colorize(f"Задача #{args.id} не найдена.", 'red'))
        return
    if task.status == 'done':
        print(colorize(f"Задача #{args.id} уже выполнена.", 'yellow'))
        return
    task.status = 'done'
    save_tasks(tasks)
    print(colorize(f"✅ Задача #{args.id} отмечена как выполненная.", 'green'))

def delete_task(args):
    tasks = load_tasks()
    task = next((t for t in tasks if t.id == args.id), None)
    if not task:
        print(colorize(f"Задача #{args.id} не найдена.", 'red'))
        return
    tasks.remove(task)
    save_tasks(tasks)
    print(colorize(f"🗑️ Задача #{args.id} удалена.", 'yellow'))

def edit_task(args):
    tasks = load_tasks()
    task = next((t for t in tasks if t.id == args.id), None)
    if not task:
        print(colorize(f"Задача #{args.id} не найдена.", 'red'))
        return
    if args.title:
        task.title = args.title
    if args.description:
        task.description = args.description
    if args.priority:
        task.priority = args.priority
    if args.deadline:
        task.deadline = args.deadline
    save_tasks(tasks)
    print(colorize(f"✅ Задача #{args.id} обновлена.", 'green'))

def search_tasks(args):
    tasks = load_tasks()
    query = args.query.lower()
    results = [t for t in tasks if query in t.title.lower() or query in t.description.lower()]
    if not results:
        print(colorize(f"Ничего не найдено по запросу '{args.query}'.", 'yellow'))
        return
    print(colorize(f"🔍 Найдено {len(results)} задач:", 'bold'))
    for task in results:
        print(f"  {task}")

def stats(args):
    tasks = load_tasks()
    total = len(tasks)
    done = len([t for t in tasks if t.status == 'done'])
    pending = total - done
    print(colorize("📊 Статистика:", 'bold'))
    print(f"  Всего задач: {total}")
    print(f"  Выполнено: {done} ({done/total*100:.1f}%)" if total else "  Выполнено: 0")
    print(f"  Ожидают: {pending}")

def export_tasks(args):
    tasks = load_tasks()
    with open(args.file, 'w') as f:
        json.dump([t.to_dict() for t in tasks], f, indent=2)
    print(colorize(f"✅ Задачи экспортированы в {args.file}", 'green'))

def import_tasks(args):
    with open(args.file, 'r') as f:
        data = json.load(f)
    tasks = load_tasks()
    for d in data:
        task = Task.from_dict(d)
        task.id = get_next_id(tasks)
        tasks.append(task)
    save_tasks(tasks)
    print(colorize(f"✅ Импортировано {len(data)} задач из {args.file}", 'green'))

def main():
    parser = argparse.ArgumentParser(description="TodoMaster – Управление задачами")
    subparsers = parser.add_subparsers(dest='command', help='Команды')

    # add
    add_parser = subparsers.add_parser('add', help='Добавить задачу')
    add_parser.add_argument('title', help='Заголовок')
    add_parser.add_argument('description', nargs='?', default='', help='Описание')
    add_parser.add_argument('-p', '--priority', choices=['High', 'Medium', 'Low'], default='Medium', help='Приоритет')
    add_parser.add_argument('-d', '--deadline', help='Дедлайн (YYYY-MM-DD)')

    # list
    list_parser = subparsers.add_parser('list', help='Список задач')
    list_parser.add_argument('--filter', help='Фильтр (status=pending|done, priority=High|Medium|Low)')
    list_parser.add_argument('--sort', choices=['priority', 'deadline', 'created'], help='Сортировка')

    # done
    done_parser = subparsers.add_parser('done', help='Отметить как выполненную')
    done_parser.add_argument('id', type=int, help='ID задачи')

    # delete
    del_parser = subparsers.add_parser('delete', help='Удалить задачу')
    del_parser.add_argument('id', type=int, help='ID задачи')

    # edit
    edit_parser = subparsers.add_parser('edit', help='Редактировать задачу')
    edit_parser.add_argument('id', type=int, help='ID задачи')
    edit_parser.add_argument('--title', help='Новый заголовок')
    edit_parser.add_argument('--description', help='Новое описание')
    edit_parser.add_argument('--priority', choices=['High', 'Medium', 'Low'], help='Новый приоритет')
    edit_parser.add_argument('--deadline', help='Новый дедлайн')

    # search
    search_parser = subparsers.add_parser('search', help='Поиск')
    search_parser.add_argument('query', help='Ключевое слово')

    # stats
    subparsers.add_parser('stats', help='Статистика')

    # export
    export_parser = subparsers.add_parser('export', help='Экспорт')
    export_parser.add_argument('file', help='Файл для экспорта')

    # import
    import_parser = subparsers.add_parser('import', help='Импорт')
    import_parser.add_argument('file', help='Файл для импорта')

    args = parser.parse_args()

    if args.command == 'add':
        add_task(args)
    elif args.command == 'list':
        list_tasks(args)
    elif args.command == 'done':
        done_task(args)
    elif args.command == 'delete':
        delete_task(args)
    elif args.command == 'edit':
        edit_task(args)
    elif args.command == 'search':
        search_tasks(args)
    elif args.command == 'stats':
        stats(args)
    elif args.command == 'export':
        export_tasks(args)
    elif args.command == 'import':
        import_tasks(args)
    else:
        parser.print_help()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print(colorize("\nВыход.", 'yellow'))
        sys.exit(0)
