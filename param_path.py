

# returns a path object that will then be given as argument
# to all other functions
pi = 3.141592

degtorad = pi / 180.0

def path_init():
    return -180.0 * degtorad

# from a path state, give a dictionary of parameter values.
# unmentioned parameters are set to default value.
def path_get_params(path):
    return {
        "rot.y":float(path)
    }

# advances a path by returning the next step
def path_advance(path):
    return path + 0.1 * degtorad

def path_is_complete(path):
    return path > (180.0*degtorad)

def path_get_colnames(path):
    return ["a"]

def path_get_colvals(path):
    return [float(path)]