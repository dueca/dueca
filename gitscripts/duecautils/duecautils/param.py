import re

class Param:

    def __init__(self, par, default_strip=''):
        """Create a new parameter from an xml definition

        Arguments:
            par -- xml object, needs as minimum attribute 'name',
                   optionally
                   * 'strip' -> do stripping,
                   * 'regex' -> treat as regular expression

        Keyword Arguments:
            default_strip -- set the strip value default
        """

        if isinstance(par, str):
            self.val = par
            self.name = 'anon'
            return

        self.name = par.get('name')
        _regex = par.get('regex', False)
        pstrip = par.get('strip', (_regex and 'both') or default_strip)

        if pstrip.lower() == 'left':
            pval = par.text.lstrip()
        elif pstrip.lower() == 'right':
            pval = par.text.rstrip()
        elif pstrip.lower() == 'true' or pstrip.lower() == 'both':
            pval = par.text.strip()
        else:
            pval = par.text

        if _regex:
            self.val = re.compile(pval)
        else:
            self.val = pval

    def match(self, val: str):
        if isinstance(self.val, re.Pattern):
            return self.val.match(val)
        else:
            return self.val == val

    def value(self, val: str=None):
        if isinstance(self.val, str) and (val is None or val == self.val):
            return self.val
        if isinstance(self.val, re.Pattern) and val is not None:
            if self.val.match(val):
                return val
            raise ValueError(f'Regex parameter {self.name} has no match on {val}')
        raise ValueError(f'Parameter {self.name}; {self.val} != {val}')

    def __str__(self):
        if isinstance(self.val, str): return self.val
        raise ValueError("Cannot get string value from regex")

    def __repr__(self):
        if isinstance(self.val, str): return self.val
        return str(self.val)

    def __bool__(self):
        if isinstance(self.val, str): return len(self.val) > 0
        return True