

pi = 3.141592

degtorad = pi / 180.0
radtodeg = 180.0 / pi


# returns a path object that will then be given as argument
# to all other functions
def path_init():
    return (-0.0*degtorad,-0.0*degtorad, 1.0)

# from a path state, give a dictionary of parameter values.
# unmentioned parameters are set to default value.
def path_get_params(path):
    return {
        "rot.y":path[0],
        "rot.x":path[1]

    }

spacing = 0.0*degtorad
# advances a path by returning the next step
def path_advance(path):
    vert, pitch, direction = path
    vert += spacing*direction

    if vert >= 0.0*degtorad:
        vert = -180.0*degtorad
        pitch -= spacing*direction

    return (vert, pitch, direction)

def path_is_complete(path):
    return abs(path[1]) > 20.0*degtorad

def path_get_colnames(path):
    return ["yaw", "pitch"]

def path_get_colvals(path):
    return [path[0],path[1]]