import numpy as np
import series_functions as series
from warnings import warn


def KLD_numerical(p1,p2, check_probs_sum_1 = True):
    p1 = np.array(p1)
    p2 = np.array(p2)
    if check_probs_sum_1:
        assert np.isclose(1,sum(p1))
        if not np.isclose(1,sum(p2)):
            warn("sum(p2) is not close to 1. Result: "+str(sum(p2)))
            
    assert np.isclose(sum(p1),sum(abs(p1))) # check the are no neg probs
    assert np.isclose(sum(p2),sum(abs(p2))) # check the are no neg probs
    return sum( p/sum(p1)*np.log2(p/q) for p,q in zip(p1,p2) if p!=0 and q!=0 )

def Entropy_numerical(p):
    p = np.array(p)
    assert np.isclose(1,sum(p))
    assert np.isclose(1,sum(abs(p))) # check the are no neg probs
    return sum(-pe*np.log2(pe) for pe in p if pe >0 )

def TSG_P(x,theta,s):
    s = -s
    x = -x
    return (1-theta)/(theta**(1-s)+ theta**s)*theta**abs(x+s)

def Geometric_P(x,theta):
    '''Returns probability of x for x~Geometric(theta) 
     that is P(x)= (1-theta)*theta**abs(x)
    '''
    assert x>=0
    assert x == int(x)
    assert 0<=theta<=1
    return (1-theta)*theta**abs(x)

def Geometric_range(theta,l,h):
    '''Returns probability of (l<=x<h) for x~Geometric(theta) 
     for P(x)= (1-theta)*theta**abs(x)
    '''
    assert 0<=theta<=1
    assert l == int(l)
    assert h == int(h)
    assert l<=h
    
    return theta**l -theta**h
    
    
def TSG2_P(x,theta,p):
    assert 0<=p<=1
    y = 1 if x < 0 else 1
    z = abs(x) -y
    prob_y = p if y ==1 else (1-p)
    prob_z = Geometric_P(z,theta)
    return prob_y*prob_z
    
    
def Bernoulli_entropy(p):
    assert 0<=p<=1
    if p == 1 or p == 0:
        return 0
    return -p*np.log2(p) -(1-p)*np.log2(1-p)

def Bernoulli_KLD(p,p_ap):
    ''' p is the actual parameter of the data and p_ap the prob the coder is tuned to'''
    assert 0<=p<=1
    if p == 1 or p == 0:
        return 0
    return p*np.log2(p/p_ap) +(1-p)*np.log2((1-p)/(1-p_ap))

def Geometric_entropy(theta,Max_x=None):
    lg2 = np.log2
    if Max_x is None:
        return -lg2(1-theta)-lg2(theta)*theta/(1-theta)
    assert Max_x == int(Max_x)
    assert Max_x >= 0
    return -lg2(1-theta)-lg2(theta)*series.Arith_Geo_series(theta,Max_x)/series.Geo_series(theta,Max_x)
    
def Geometric_KLD(theta,theta_ap,Max_x=None):
    assert 0<=theta<=1
    assert 0<=theta_ap<=1
    if Max_x is None:
        return np.log2((1-theta)/(1-theta_ap))+np.log2(theta/theta_ap)*theta/(1-theta)
    
    assert Max_x == int(Max_x)
    assert Max_x > 0
    return (np.log2((1-theta)/(1-theta_ap))+ 
                np.log2(theta/theta_ap)*series.Arith_Geo_series(theta,Max_x)/series.Geo_series(theta,Max_x) )  

def Geometric_KLD_from_St(St,St_ap):
    assert St>=0 
    assert St_ap>=0 
    lg2 = np.log2
    return (St+1)*(lg2(St_ap+1)-lg2(St+1)) + St*(lg2(St)-lg2(St_ap) )



if __name__ == '__main__':

    def Test_Geometric_prob():
        print("Test Geometric prob functions:",end="")

        theta_test = [.1,.3,.5,.7]
        for theta in theta_test:
            for low in [0,3,5]:
                for high in low+np.array([1,5,7]):
                    numeric_sum = sum(Geometric_P(x,theta) for x in range(low,high))
                    assert np.isclose(numeric_sum,Geometric_range(theta,low,high))
        print(" OK")
    Test_Geometric_prob()


    def Test_Geometric_Entropy():
        print("Test Geometric Entropy functions:",end="")
        numeric_prob_tol = 1e-10
        theta_test = [.1,.3,.5,.7]
        for theta in theta_test:
            range_theta = int(np.log10(numeric_prob_tol)/np.log10(theta))
            symb_prob_1 = [Geometric_P(x,theta) for x in range(range_theta+1)]

            num_E = Entropy_numerical(symb_prob_1)
            theo_E = Geometric_entropy(theta)
            assert np.isclose(num_E,theo_E),"Fails for: theta ={}".format(theta)
            assert theo_E >= 0, "Entropy can\t be negative"


        print(" OK")
    Test_Geometric_Entropy()



    def Test_Geometric_KLD():
        print("Test Geometric KLD functions:",end="")
        numeric_prob_tol = 1e-10
        theta_test = [.1,.3,.5,.7]
        for Max_x in [1,4,10,None]:
            for theta in theta_test:
                for theta_ap in [.2,.24,.5,.8]:
                    range_theta = Max_x
                    check_probs_sum_1 = False
                    if Max_x is None:
                        check_probs_sum_1 = True
                        max_theta = max(theta,theta_ap)
                        range_theta = int(np.log10(numeric_prob_tol)/np.log10(max_theta))
                    symb_prob_1 = [Geometric_P(x,theta) for x in range(range_theta+1)]
                    symb_prob_2 = [Geometric_P(x,theta_ap) for x in range(range_theta+1)]
                    num_kld = KLD_numerical(symb_prob_1,symb_prob_2,check_probs_sum_1)
                    theo_kld = Geometric_KLD(theta,theta_ap,Max_x)
                    assert np.isclose(num_kld,theo_kld), (
                        "Fails for Max_x={}, theta={}, theta_ap={} ".format(Max_x,theta,theta_ap) + 
                        "\n| theo_kld={:.5f}, num_kld={:.5f}, error={:.4e} ".format(theo_kld,num_kld,theo_kld-num_kld))
                    if Max_x is None:
                        assert theo_kld >= 0, ("KLD can\t be negative" + 
                                           "\n| theo_kld={:.5f} ".format(theo_kld))
        print(" OK")

    Test_Geometric_KLD()


    def Test_Bernoulli_Entropy():
        print("Test Bernoulli Entropy functions:",end="")
        p_test = [.1,.3,.5,.7]
        for p in p_test:
            num_E = Entropy_numerical([p , 1-p])
            theo_E = Bernoulli_entropy(p)
            assert np.isclose(num_E,theo_E),"Fails for: p ={}".format(p)
            assert theo_E >= 0, "Entropy can\t be negative"


        print(" OK")
    Test_Bernoulli_Entropy()

    def Test_Bernoulli_KLD():
        print("Test Bernoulli KLD functions:",end="")
        p_test = [.1,.3,.5,.7]
        for p in p_test:
            for p_ap in [.2,.24,.5,.8]:
              num_kld = KLD_numerical([p , 1-p],[p_ap , 1-p_ap])
              theo_kld = Bernoulli_KLD(p,p_ap)
              assert np.isclose(num_kld,theo_kld),"Fails for: p ={}".format(p)
              assert theo_kld >= 0, "KLD can\t be negative"
        print(" OK")

    Test_Bernoulli_KLD()
