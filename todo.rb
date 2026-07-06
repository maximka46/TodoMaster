#!/usr/bin/env ruby
# todo.rb
# encoding: UTF-8

require 'json'
require 'time'
require 'optparse'
require 'fileutils'

COLORS = {
  reset: "\e[0m",
  red: "\e[91m",
  green: "\e[92m",
  yellow: "\e[93m",
  blue: "\e[94m",
  bold: "\e[1m"
}

def colorize(text, color)
  "#{COLORS[color]}#{text}#{COLORS[:reset]}"
end

TASKS_FILE = File.join(Dir.home, '.todo_tasks.json')

def load_tasks
  return [] unless File.exist?(TASKS_FILE)
  JSON.parse(File.read(TASKS_FILE))
end

def save_tasks(tasks)
  File.write(TASKS_FILE, JSON.pretty_generate(tasks))
end

def next_id(tasks)
  tasks.empty? ? 1 : tasks.map { |t| t['id'] }.max + 1
end

def task_color(task)
  return 'green' if task['status'] == 'done'
  case task['priority']
  when 'High' then 'red'
  when 'Medium' then 'yellow'
  when 'Low' then 'blue'
  else 'reset'
  end
end

def priority_symbol(task)
  case task['priority']
  when 'High' then '🔴'
  when 'Medium' then '🟡'
  when 'Low' then '🔵'
  else '⚪'
  end
end

def add_task(title, description, priority, deadline)
  tasks = load_tasks
  task = {
    'id' => next_id(tasks),
    'title' => title,
    'description' => description || '',
    'priority' => priority || 'Medium',
    'deadline' => deadline || '',
    'status' => 'pending',
    'created_at' => Time.now.iso8601
  }
  tasks << task
  save_tasks(tasks)
  puts colorize("✅ Задача ##{task['id']} добавлена.", :green)
end

def list_tasks(filter, sort_by)
  tasks = load_tasks
  if tasks.empty?
    puts colorize('Нет задач.', :yellow)
    return
  end
  unless filter.nil?
    key, val = filter.split('=')
    tasks = tasks.select { |t| t[key] == val }
  end
  case sort_by
  when 'priority'
    order = { 'High' => 0, 'Medium' => 1, 'Low' => 2 }
    tasks.sort_by! { |t| order[t['priority']] }
  when 'deadline'
    tasks.sort_by! { |t| t['deadline'] || '9999-12-31' }
  when 'created'
    tasks.sort_by! { |t| t['created_at'] }
  end
  tasks.each do |t|
    status = t['status'] == 'done' ? '✅' : '⬜'
    deadline = t['deadline'].empty? ? '' : " до #{t['deadline']}"
    puts "[#{t['id']}] #{status} #{t['title']} #{colorize(t['priority'], task_color(t))} #{deadline}"
  end
end

def done_task(id)
  tasks = load_tasks
  task = tasks.find { |t| t['id'] == id }
  unless task
    puts colorize("Задача ##{id} не найдена.", :red)
    return
  end
  if task['status'] == 'done'
    puts colorize("Задача ##{id} уже выполнена.", :yellow)
    return
  end
  task['status'] = 'done'
  save_tasks(tasks)
  puts colorize("✅ Задача ##{id} отмечена как выполненная.", :green)
end

def delete_task(id)
  tasks = load_tasks
  task = tasks.find { |t| t['id'] == id }
  unless task
    puts colorize("Задача ##{id} не найдена.", :red)
    return
  end
  tasks.delete(task)
  save_tasks(tasks)
  puts colorize("🗑️ Задача ##{id} удалена.", :yellow)
end

def edit_task(id, title, description, priority, deadline)
  tasks = load_tasks
  task = tasks.find { |t| t['id'] == id }
  unless task
    puts colorize("Задача ##{id} не найдена.", :red)
    return
  end
  task['title'] = title unless title.nil?
  task['description'] = description unless description.nil?
  task['priority'] = priority unless priority.nil?
  task['deadline'] = deadline unless deadline.nil?
  save_tasks(tasks)
  puts colorize("✅ Задача ##{id} обновлена.", :green)
end

def search_tasks(query)
  tasks = load_tasks
  results = tasks.select do |t|
    t['title'].downcase.include?(query.downcase) ||
    t['description'].downcase.include?(query.downcase)
  end
  if results.empty?
    puts colorize("Ничего не найдено по запросу '#{query}'.", :yellow)
    return
  end
  puts colorize("🔍 Найдено #{results.size} задач:", :bold)
  results.each { |t| puts "  [#{t['id']}] #{t['title']}" }
end

def stats
  tasks = load_tasks
  total = tasks.size
  done = tasks.count { |t| t['status'] == 'done' }
  pending = total - done
  puts colorize("📊 Статистика:", :bold)
  puts "  Всего задач: #{total}"
  if total > 0
    puts "  Выполнено: #{done} (#{(done.to_f/total*100).round(1)}%)"
  else
    puts "  Выполнено: 0"
  end
  puts "  Ожидают: #{pending}"
end

def export_tasks(filename)
  tasks = load_tasks
  File.write(filename, JSON.pretty_generate(tasks))
  puts colorize("✅ Задачи экспортированы в #{filename}", :green)
end

def import_tasks(filename)
  unless File.exist?(filename)
    puts colorize("Файл не найден.", :red)
    return
  end
  imported = JSON.parse(File.read(filename))
  tasks = load_tasks
  imported.each do |t|
    t['id'] = next_id(tasks)
    tasks << t
  end
  save_tasks(tasks)
  puts colorize("✅ Импортировано #{imported.size} задач из #{filename}", :green)
end

def main
  if ARGV.empty?
    puts "Usage: todo.rb <add|list|done|delete|edit|search|stats|export|import> [args...]"
    puts "  add <title> [description] -p <priority> -d <deadline>"
    puts "  list [--filter status=pending|done] [--sort priority|deadline|created]"
    puts "  done <id>"
    puts "  delete <id>"
    puts "  edit <id> --title <title> --description <desc> --priority <p> --deadline <d>"
    puts "  search <query>"
    puts "  stats"
    puts "  export <file>"
    puts "  import <file>"
    exit 1
  end

  cmd = ARGV[0]
  case cmd
  when 'add'
    if ARGV.size < 2
      puts "Использование: add <title> [description] -p <priority> -d <deadline>"
      return
    end
    title = ARGV[1]
    description = ''
    priority = 'Medium'
    deadline = ''
    i = 2
    while i < ARGV.size
      case ARGV[i]
      when '-p' then priority = ARGV[i+1]; i += 1
      when '-d' then deadline = ARGV[i+1]; i += 1
      else description = ARGV[i] if description.empty?
      end
      i += 1
    end
    add_task(title, description, priority, deadline)

  when 'list'
    filter = nil
    sort_by = nil
    i = 1
    while i < ARGV.size
      case ARGV[i]
      when '--filter' then filter = ARGV[i+1]; i += 1
      when '--sort' then sort_by = ARGV[i+1]; i += 1
      end
      i += 1
    end
    list_tasks(filter, sort_by)

  when 'done'
    if ARGV.size < 2
      puts "Использование: done <id>"
      return
    end
    done_task(ARGV[1].to_i)

  when 'delete'
    if ARGV.size < 2
      puts "Использование: delete <id>"
      return
    end
    delete_task(ARGV[1].to_i)

  when 'edit'
    if ARGV.size < 3
      puts "Использование: edit <id> --title <title> --description <desc> --priority <p> --deadline <d>"
      return
    end
    id = ARGV[1].to_i
    title = description = priority = deadline = nil
    i = 2
    while i < ARGV.size
      case ARGV[i]
      when '--title' then title = ARGV[i+1]; i += 1
      when '--description' then description = ARGV[i+1]; i += 1
      when '--priority' then priority = ARGV[i+1]; i += 1
      when '--deadline' then deadline = ARGV[i+1]; i += 1
      end
      i += 1
    end
    edit_task(id, title, description, priority, deadline)

  when 'search'
    if ARGV.size < 2
      puts "Использование: search <query>"
      return
    end
    search_tasks(ARGV[1])

  when 'stats'
    stats

  when 'export'
    if ARGV.size < 2
      puts "Использование: export <file>"
      return
    end
    export_tasks(ARGV[1])

  when 'import'
    if ARGV.size < 2
      puts "Использование: import <file>"
      return
    end
    import_tasks(ARGV[1])

  else
    puts "Неизвестная команда: #{cmd}"
  end
end

main if __FILE__ == $0
