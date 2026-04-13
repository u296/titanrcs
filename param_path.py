

pi = 3.141592

degtorad = pi / 180.0


# returns a path object that will then be given as argument
# to all other functions
def path_init():
    return (-180.0*degtorad,0.0, 1.0)

# from a path state, give a dictionary of parameter values.
# unmentioned parameters are set to default value.
def path_get_params(path):
    return {
        "rot.y":path[0],
        "rot.x":path[1]

    }

spacing = 2*degtorad
# advances a path by returning the next step
def path_advance(path):
    vert, pitch, direction = path
    return (vert+spacing*direction, pitch, direction)

def path_is_complete(path):
    return path[0] > 180.0*degtorad

def path_get_colnames(path):
    return ["a"]

def path_get_colvals(path):
    return [path[0],path[1]]