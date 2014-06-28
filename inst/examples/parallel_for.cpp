#include <Rcpp.h>
using namespace Rcpp;

// [[Rcpp::depends(tbb)]]
#include <tbb/tbb.h>
using namespace tbb;

struct Sum {
    double value;
    Sum() : value(0) {}
    Sum( Sum& s, split ) {value = 0;}
    void operator()( const blocked_range<double*>& r ) {
        double temp = value;
        for( double* a=r.begin(); a!=r.end(); ++a ) {
            temp += *a;
        }
        value = temp;
    }
    void join( Sum& rhs ) {value += rhs.value;}
};


float ParallelSum( double* array, size_t n ) {
    Sum total;
    parallel_reduce( blocked_range<double*>( array, array+n ), 
                     total );
    return total.value;
}

// [[Rcpp::export]]
float parallelSum(NumericVector x) {
   return ParallelSum(x.begin(), x.length());
}
