import os
import typing  # required to prevent typing.Union namespace overwriting Union
from collections.abc import Iterable
import numpy as np

# Type for arguments that accept file paths
PathLike = typing.Union[str, os.PathLike]

class CheckedList(list):
    """A list for which each element is type-checked as it's added

    Parameters
    ----------
    expected_type : type or Iterable of type
        Type(s) which each element should be
    name : str
        Name of data being checked
    items : Iterable, optional
        Items to initialize the list with

    """

    def __init__(self, expected_type, name, items=None):
        super().__init__()
        self.expected_type = expected_type
        self.name = name
        if items is not None:
            for item in items:
                self.append(item)

    def __add__(self, other):
        new_instance = copy.copy(self)
        new_instance += other
        return new_instance

    def __radd__(self, other):
        return self + other

    def __iadd__(self, other):
        check_type('CheckedList add operand', other, Iterable,
                   self.expected_type)
        for item in other:
            self.append(item)
        return self

    def append(self, item):
        """Append item to list

        Parameters
        ----------
        item : object
            Item to append

        """
        check_type(self.name, item, self.expected_type)
        super().append(item)

    def insert(self, index, item):
        """Insert item before index

        Parameters
        ----------
        index : int
            Index in list
        item : object
            Item to insert

        """
        check_type(self.name, item, self.expected_type)
        super().insert(index, item)

def check_type(name, value, expected_type, expected_iter_type=None, *, none_ok=False):
    """Ensure that an object is of an expected type. Optionally, if the object is
    iterable, check that each element is of a particular type.

    Parameters
    ----------
    name : str
        Description of value being checked
    value : object
        Object to check type of
    expected_type : type or Iterable of type
        type to check object against
    expected_iter_type : type or Iterable of type or None, optional
        Expected type of each element in value, assuming it is iterable. If
        None, no check will be performed.
    none_ok : bool, optional
        Whether None is allowed as a value

    """
    if none_ok and value is None:
        return

    if not isinstance(value, expected_type):
        if isinstance(expected_type, Iterable):
            msg = 'Unable to set "{}" to "{}" which is not one of the ' \
                  'following types: "{}"'.format(name, value, ', '.join(
                      [t.__name__ for t in expected_type]))
        else:
            msg = (f'Unable to set "{name}" to "{value}" which is not of type "'
                   f'{expected_type.__name__}"')
        raise TypeError(msg)

    if expected_iter_type:
        if isinstance(value, np.ndarray):
            if not issubclass(value.dtype.type, expected_iter_type):
                msg = (f'Unable to set "{name}" to "{value}" since each item '
                       f'must be of type "{expected_iter_type.__name__}"')
                raise TypeError(msg)
            else:
                return

        for item in value:
            if not isinstance(item, expected_iter_type):
                if isinstance(expected_iter_type, Iterable):
                    msg = 'Unable to set "{}" to "{}" since each item must be ' \
                          'one of the following types: "{}"'.format(
                              name, value, ', '.join([t.__name__ for t in
                                                      expected_iter_type]))
                else:
                    msg = (f'Unable to set "{name}" to "{value}" since each '
                           f'item must be of type "{expected_iter_type.__name__}"')
                raise TypeError(msg)

def check_greater_than(name, value, minimum, equality=False):
    """Ensure that an object's value is greater than a given value.

    Parameters
    ----------
    name : str
        Description of the value being checked
    value : object
        Object to check
    minimum : object
        Minimum value to check against
    equality : bool, optional
        Whether equality is allowed. Defaults to False.

    """

    if equality:
        if value < minimum:
            msg = (f'Unable to set "{name}" to "{value}" since it is less than '
                   f'"{minimum}"')
            raise ValueError(msg)
    else:
        if value <= minimum:
            msg = (f'Unable to set "{name}" to "{value}" since it is less than '
                   f'or equal to "{minimum}"')
            raise ValueError(msg)

def check_value(name, value, accepted_values):
    """Ensure that an object's value is contained in a set of acceptable values.

    Parameters
    ----------
    name : str
        Description of value being checked
    value : collections.Iterable
        Object to check
    accepted_values : collections.Container
        Container of acceptable values

    """

    if value not in accepted_values:
        msg = (f'Unable to set "{name}" to "{value}" since it is not in '
               f'"{accepted_values}"')
        raise ValueError(msg)
