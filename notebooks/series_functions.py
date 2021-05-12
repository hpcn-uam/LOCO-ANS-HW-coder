import numpy as np

def Geo_series(q,N=None):
    assert 0<=q<1
    if N is None:
        return 1/(1-q)
    
    assert N >=0
    assert N == int(N)
    return (1-q**(N+1))/(1-q)

def Geo_series_numeric(q,N=None):
    assert 0<q<1
    if N is None:
        N = int(np.log10(1e-10*(1-q))/np.log10(q))
    assert N >=0
    assert N == int(N)
    return sum(q**x for x in range(N+1))

def Arith_Geo_series(q,N=None):
    assert 0<=q<1
    if N is None:
        return q/(1-q)**2
    
    assert N >=0
    assert N == int(N)
    return (q-q**(N+1))/(1-q)**2 - N*q**(N+1)/(1-q)

def Arith_Geo_series_numeric(q,N=None):
    assert 0<q<1
    if N is None:
        N = int(np.log10(1e-10*(1-q))/np.log10(q))
    assert N >=0
    assert N == int(N)
    return sum(x*q**x for x in range(1,N+1))


if __name__ == '__main__':
    
    def Test_Geo_series():
        print("Test Geometric series functions:",end="")
        for q in [.01,.3,.5]:
            for N in [0,1,5,20,100,None]:
                assert  np.isclose(Geo_series(q,N),Geo_series_numeric(q,N)), "q:{} |N:{}".format(q,N)
        print("OK")

    def Test_Arith_Geo_series():
        print("Test Arithmetic Geometric series functions:",end="")
        for q in [.01,.3,.5]:
            for N in [0,1,5,20,100,None]:
                assert  np.isclose(Arith_Geo_series(q,N),Arith_Geo_series_numeric(q,N)), "q:{} |N:{}".format(q,N)
        print("OK")

    Test_Geo_series()
    Test_Arith_Geo_series()   