#ifndef RCPP_PARALLEL_SIMD_ALGORITHM_H
#define RCPP_PARALLEL_SIMD_ALGORITHM_H

namespace RcppParallel {

template <typename T, typename U, typename UnOp>
U* simdTransform(const T* begin, const T* end, U* out, UnOp&& f)
{
   return boost::simd::transform(begin, end, out, std::forward<UnOp>(f));
}

template <typename T1, typename T2, typename U, typename BinOp>
U* simdTransform(const T1* begin1, const T1* end1, const T2* begin2, U* out, BinOp&& f)
{
   return boost::simd::transform(begin1, end1, begin2, out, std::forward<BinOp>(f));
}

template <typename T, typename U, typename F>
U simdAccumulate(const T* begin, const T* end, U init, F&& f)
{
   return boost::simd::accumulate(begin, end, init, std::forward<F>(f));
}

template <typename T, typename F>
F simdFor(const T* it, const T* end, F&& f)
{
   typedef boost::simd::pack<T> vT;
   static const std::size_t N = vT::static_size;
   const T* aligned_begin = std::min(boost::simd::align_on(it, N * sizeof(T)), end);
   const T* aligned_end   = aligned_begin + (end - aligned_begin) / N * N;
   
   for (; it != aligned_begin; ++it)
      std::forward<F>(f)(*it);
   
   for (; it != aligned_end; it += N)
      std::forward<F>(f)(boost::simd::aligned_load<vT>(it));
   
   for (; it != end; ++it)
      std::forward<F>(f)(*it);
   
   return f;
}

template <typename T, typename U, typename F>
U simdReduce(const T* begin, const T* end, U init, F f)
{
   return boost::simd::accumulate(begin, end, init, f);
}

template <typename T, typename U, typename MapReducer>
U simdMapReduce(const T* it, const T* end, U init, MapReducer mapper)
{
   // We separate the range into three regions, to allow
   // for aligned loads of data (when possible), and scalar
   // reads otherwise.
   //
   // --------- | ------- | ---------
   // unaligned | aligned | unaligned
   //
   //           ^ ------------------- aligned_begin
   //                     ^---------- aligned_end
   
   typedef boost::simd::pack<T> vT; // SIMD vector of T
   typedef boost::simd::pack<U> vU; // SIMD vector of U
   
   static const std::size_t N = vT::static_size;
   const T* aligned_begin = std::min(boost::simd::align_on(it, N * sizeof(T)), end);
   const T* aligned_end   = aligned_begin + (end - aligned_begin) / N * N;
   
   // Buffer for the SIMD mapping operations
   vU buffer = boost::simd::splat<vU>(init);
   
   // Scalar operations for the initial unaligned region
   for (; it != aligned_begin; ++it)
      mapper.map(*it, &init);
   
   // Aligned, SIMD operations
   for (; it != aligned_end; it += N)
      mapper.map(boost::simd::aligned_load<vT>(it), &buffer);
   
   // Reduce the buffer, joining it into the scalar vale
   mapper.reduce(buffer, &init);
   
   // Leftover unaligned region
   for (; it != end; ++it)
      mapper.map(*it, &init);
   
   return init;
}

} // namespace RcppParallel

#endif /* RCPP_PARALLEL_SIMD_ALGORITHM_H */