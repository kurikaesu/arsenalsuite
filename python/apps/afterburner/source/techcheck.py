import os

def checkSpace(location):

    space = os.statvfs(location)

    # Total space
    total = (space.f_favail * space.f_frsize) / 1024.0

    # Available space
    available = (space.f_bavail * space.f_frsize) / 1024.0

    # Percentage
    percent = int((available / total) * 100)

    return percent

def check():

    # Check if there is enough space left on /Local
    if checkSpace('/Local') < 10:
        return False

if __name__ == '__main__':

    check()
