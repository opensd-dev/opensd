

class Result:

    def __init__(self,resfile):
        self.resfile = resfile
    
    def get_series(self,parameter):
        from matplotlib import pyplot
        import pandas as pd
        
        df = pd.read_csv(self.resfile,header=0,delimiter=' ',skipinitialspace = True)
        
        df.plot(x="time(s)",y=parameter)
        
        return pyplot
