import sys
from queue import LifoQueue, Queue
import API
import location
import state

MAZE_WIDTH = 16
MAZE_HEIGHT = 16

cur_direction = 0
cur_position = [0, 0]
maze = [[location.Location([i, j]) for j in range(0, MAZE_WIDTH)] for i in range(0, MAZE_HEIGHT)]

loc_stack = LifoQueue()
dir_stack = LifoQueue()
act_stack = LifoQueue()
frontier = Queue()

def update_position(move_direction=1):
    global cur_position
    if cur_direction == 0:
        cur_position[1] += move_direction
    elif cur_direction == 1:
        cur_position[0] += move_direction
    elif cur_direction == 2:
        cur_position[1] -= move_direction
    elif cur_direction == 3:
        cur_position[0] -= move_direction

def update_direction(turn_direction):
    global cur_direction
    cur_direction = (cur_direction + turn_direction) % 4

def get_walls():
    walls = [False, False, False, False]
    walls[cur_direction] = API.wallFront()
    walls[(cur_direction + 1) % 4] = API.wallRight()
    walls[(cur_direction + 2) % 4] = False
    walls[(cur_direction + 3) % 4] = API.wallLeft()
    if cur_position == [0, 0]:
        walls[2] = True
    return walls

def mark_visited_api(pos=None):
    if pos is None:
        pos = cur_position
    API.setColor(pos[0], pos[1], "G")
    API.setText(pos[0], pos[1], "hit")

def mark_solution_api(pos=None):
    if pos is None:
        pos = cur_position
    API.setColor(pos[0], pos[1], "B")
    API.setText(pos[0], pos[1], "Sol")

def mark_bfs_api(pos=None):
    if pos is None:
        pos = cur_position
    API.setColor(pos[0], pos[1], "c")
    API.setText(pos[0], pos[1], "dfs")

def mark_bktrk_api(pos=None):
    if pos is None:
        pos = cur_position
    API.setColor(pos[0], pos[1], "o")
    API.setText(pos[0], pos[1], "bktrk")

def log(string):
    sys.stderr.write("{}\n".format(string))
    sys.stderr.flush()

def move_forward():
    API.moveForward()
    update_position(+1)

def turn_left():
    API.turnLeft()
    update_direction(-1)

def turn_right():
    API.turnRight()
    update_direction(+1)

def turn_around():
    turn_right()
    turn_right()

def set_dir(_dir):
    if _dir == cur_direction:
        return
    if _dir == (cur_direction + 1) % 4:
        turn_right()
        return
    if _dir == (cur_direction + 2) % 4:
        turn_right()
        turn_right()
        return
    turn_left()
    return

def turn_toward(loc):
    _dir = cur_direction
    if cur_position[0] == loc.position[0]:
        if cur_position[1] - loc.position[1] == 1:
            _dir = 2
        else:
            _dir = 0
    else:
        if cur_position[0] - loc.position[0] == 1:
            _dir = 3
        else:
            _dir = 1
    set_dir(_dir)

def dfs_map_maze():
    cur_loc = maze[cur_position[0]][cur_position[1]]

    if not cur_loc.visited:
        cur_loc.set_visited(True)
        cur_loc.set_walls(get_walls())
        mark_visited_api(cur_position)

        if not cur_loc.walls[0] and not maze[cur_position[0]][cur_position[1] + 1].visited:
            loc_stack.put(maze[cur_position[0]][cur_position[1] + 1])

        if not cur_loc.walls[1] and not maze[cur_position[0] + 1][cur_position[1]].visited:
            loc_stack.put(maze[cur_position[0] + 1][cur_position[1]])

        if not cur_loc.walls[2] and not maze[cur_position[0]][cur_position[1] - 1].visited:
            loc_stack.put(maze[cur_position[0]][cur_position[1] - 1])

        if not cur_loc.walls[3] and not maze[cur_position[0] - 1][cur_position[1]].visited:
            loc_stack.put(maze[cur_position[0] - 1][cur_position[1]])

    while True:
        if loc_stack.empty():
            if not cur_position == [0, 0]:
                set_dir((dir_stack.get() + 2) % 4)
                move_forward()
                dfs_map_maze()
            return
        next_loc = loc_stack.get()
        if not next_loc.visited:
            break

    if cur_loc.can_move_to(next_loc):
        turn_toward(next_loc)
        dir_stack.put(cur_direction)
        move_forward()
    else:
        loc_stack.put(next_loc)
        set_dir((dir_stack.get() + 2) % 4)
        move_forward()
    dfs_map_maze()

def find_bfs_shortest_path():
    for i in range(0, MAZE_HEIGHT):
        for j in range(0, MAZE_WIDTH):
            maze[i][j].visited = False
    first_state = state.State(maze[0][0])
    frontier.put(first_state)
    while not frontier.empty():
        next_state = frontier.get()
        maze[next_state.location.position[0]][next_state.location.position[1]].set_visited(True)
        mark_bfs_api(next_state.location.position)
        if next_state.is_goal():
            return next_state
        my_loc = next_state.location
        if not my_loc.walls[0]:
            north_loc = maze[my_loc.position[0]][my_loc.position[1] + 1]
        if not my_loc.walls[1]:
            east_loc  = maze[my_loc.position[0] + 1][my_loc.position[1]]
        if not my_loc.walls[2]:
            south_loc = maze[my_loc.position[0]][my_loc.position[1] - 1]
        if not my_loc.walls[3]:
            west_loc  = maze[my_loc.position[0] - 1][my_loc.position[1]]

        if not my_loc.walls[0] and my_loc.can_move_to(north_loc) and not north_loc.visited:
            north_state = state.State(north_loc, next_state, (0 - next_state.cur_dir) % 4, 0)
            frontier.put(north_state)

        if not my_loc.walls[1] and my_loc.can_move_to(east_loc) and not east_loc.visited:
            east_state = state.State(east_loc, next_state, (1 - next_state.cur_dir) % 4, 1)
            frontier.put(east_state)

        if not my_loc.walls[2] and my_loc.can_move_to(south_loc) and not south_loc.visited:
            south_state = state.State(south_loc, next_state, (2 - next_state.cur_dir) % 4, 2)
            frontier.put(south_state)

        if not my_loc.walls[3] and my_loc.can_move_to(west_loc) and not west_loc.visited:
            west_state = state.State(west_loc, next_state, (3 - next_state.cur_dir) % 4, 3)
            frontier.put(west_state)

def execute_shortest_path(sol):
    while sol.parent is not sol:
        act_stack.put(sol.action)
        mark_bktrk_api(sol.location.position)
        sol = sol.parent
    while not act_stack.empty():
        act = act_stack.get()
        mark_solution_api()
        if act is 1:
            turn_right()
        elif act is 3:
            turn_left()
        move_forward()

def main():
    log("Running...")
    dfs_map_maze()
    set_dir(0)
    solution = find_bfs_shortest_path()
    execute_shortest_path(solution)
    log("Done!")

if __name__ == "__main__":
    main()
