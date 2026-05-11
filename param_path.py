

pi = 3.141592

degtorad = pi / 180.0
radtodeg = 180.0 / pi

lam = 0.1

def path_datawrite_settings():
    real = {
        "filename":f"presdata/{int(100*lam)}cm.csv",
        "overwrite":False
    }

    return {}

# returns a path object that will then be given as argument
# to all other functions
def path_init():
    return (0*degtorad,-5*degtorad, 1.0)

# from a path state, give a dictionary of parameter values.
# unmentioned parameters are set to default value.
def path_get_params(path):
    return {
        "rot.y":path[0],
        "rot.x":path[1],
        "pol_angle":0,
        "lambda":lam,
        #"po_enable":True,
    }

spacing = 0.1*degtorad
# advances a path by returning the next step
def path_advance(path):
    vert, pitch, direction = path
    vert += spacing*direction

    if vert >= 180.0*degtorad:
        vert = 0.0*degtorad
        pitch -= spacing*direction

    return (vert, pitch, direction)

def path_is_complete(path):
    vert, pitch, direction = path
    return pitch <= -25.0*degtorad

def path_get_colnames(path):
    return ["yaw", "pitch"]

def path_get_colvals(path):
    return [path[0],path[1]]