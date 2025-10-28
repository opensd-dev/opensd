# stats.py
import bisect
import lxml.etree as ET

class Tabular(object):
    """
    Represents a time-dependent tabular function.
    """

    def __init__(self, times, values, interp="linear"):
        """
        Parameters
        ----------
        times : list[float]
            List of time points (must be sorted ascending)
        values : list[float]
            Corresponding values at each time
        interp : str
            Interpolation type (currently supports 'linear' or 'step')
        """
        assert len(times) == len(values), "times and values must have same length"
        assert all(t2 >= t1 for t1, t2 in zip(times, times[1:])), "times must be sorted"
        self.times = times
        self.values = values
        self.interp = interp

    def eval(self, t):
        """Return interpolated value at time t."""
        if t <= self.times[0]:
            return self.values[0]
        if t >= self.times[-1]:
            return self.values[-1]

        i = bisect.bisect_right(self.times, t) - 1
        t1, t2 = self.times[i], self.times[i+1]
        v1, v2 = self.values[i], self.values[i+1]

        if self.interp == "linear":
            return v1 + (v2 - v1) * (t - t1) / (t2 - t1)
        elif self.interp == "step":
            return v1
        else:
            raise ValueError(f"Unknown interpolation: {self.interp}")

    def to_xml_element(self, element):
        """Write this tabular data as XML (used by Action)."""
        tabular_elem = ET.SubElement(element, "tabular")
        tabular_elem.set("interp", self.interp)
        for t, v in zip(self.times, self.values):
            point_elem = ET.SubElement(tabular_elem, "point")
            point_elem.set("time", str(t))
            point_elem.set("value", str(v))
        return tabular_elem
