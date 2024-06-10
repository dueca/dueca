from .policycondition import PolicyCondition, checkAndSet
from ..matchreference import MatchReferenceDco
from ..param import Param


class MatchFunctionDCO:
    """ Function object indicating a match

        The project supplying the module matches the project supplying the
        DCO object, optionally, the dco object also matches a given regex
        or string.
    """

    # match union
    matchon = set(('project', 'dco'))
    forall = set(('module',))

    def __init__(self, project, dco):
        """Create a match check

        Parameters
        ----------
        project : Param
            name/regex of the project to match/filter
        dco : Param
            name/regex of the the dco
        """
        self.projectref = project
        self.dcoref = dco

    def __call__(self, project, dco, **kwargs):
        if self.projectref is None or self.dcoref is None:
            return False

        if project is not None and dco is not None:
            return self.projectref.match(project) and \
                self.dcoref.match(dco)
        return False

    def explain(self, project=None, dco=None, **kwargs):
        if project is None and dco is None:
            return f'FALSE, no match on {self.projectref.val} / {self.dcoref.val}'
        return f"Match: project '{project}' ~ '{self.projectref.val}'" + \
            f" dco '{dco}' ~ '{self.dcoref.val}'"


class HomeDco(PolicyCondition):
    """ Check origin of all DCO in a comm-objects.lst

       Test to see whether a dco object's parent project is the same as
       the project from which the module with the comm-objects.lst
       is borrowed.

       The test produces true if that is so for any of the dco in a list

    Parent
    ------
    PolicyCondition : Generic test parent class
    """

    # resulting match type
    matchresult = MatchReferenceDco

    # Determine how param arguments need to be stripped
    default_strip = dict(dco='both', resultvar='both')

    def __init__(self, dco=None, resultvar=None, **kwargs):
        """
        Test whether a dco object is "home".

        Parameters
        ----------
        dco : Param|None
            Dco object name or regex (without .dco suffix).
        resultvar : str, optional
            Result variable name. Details of the check Will be passed on
            to remaining checks and actions as a list of MatchReferenceDco
            objects, one for each comm-objects.lst file
        **kwargs : dict
            Remaining, unused variables.

        Returns
        -------
        None.

        """
        self.dco = dco
        self.resultvar = resultvar

    def holds(self, p_project, p_commobjects, **kwargs):
        """Check whether a specific module uses a DCO file

        Arguments:
            p_commobjects -- dictionary, keyed with module names, one
                             per comm-objects.lst file
            p_project     -- home project hosting the module

        Returns:
            Tuple (bool: true if any match found,
            list explaining all maching dcos,
            dict with new variables for further processing
            )
        """

        res = list()
        newvars = dict()

        # each item in p_commobjects is the dco filename and list of
        # project/dco or comment lines in that dco files
        for m, commobj in p_commobjects.items():
            res.append(MatchReferenceDco(
                MatchFunctionDCO(Param(p_project), self.dco),
                                       commobjects=commobj))

        # the MatchReferenceDco objects have a truthy value, if any of the
        # dco match the isMatch function

        # if applicable assemble any result in newvars under the resultvar name
        checkAndSet(self.resultvar, newvars, res)

        result = [ r for r in res if r.value ]
        # return true or false for the whole, a list of information on the
        # matches, and the new variables
        return (result, map(self.__class__.matchresult.explain, result), newvars)

PolicyCondition.register("home-dco", HomeDco)
