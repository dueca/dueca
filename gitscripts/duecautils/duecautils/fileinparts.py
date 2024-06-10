class Line:

    def __init__(self, line):
        self._line = line
        self.value = 0

    def line(self):
        return self._line

class FileInParts:

    def __init__(self, path:str):

        self.fname = path
        self.clean = None
        self._sync()

    def _sync(self):

        if self.clean:
            return

        if self.clean is None:
            # dprint("Reading", self.fname)
            with open(self.fname, 'r') as cf:
                self.lines = [ Line(l) for l in cf ]
            self.clean = True
            # dprint("after reading", list(map(str, self.dco)))
            return

        # dirty, write now
        try:
            os.rename(self.fname, self.fname + '~')
        except FileNotFoundError:
            pass

        with open(self.fname, 'w') as cf:
            for l in self.dco:
                cf.write(l.line())
        self.clean = True
