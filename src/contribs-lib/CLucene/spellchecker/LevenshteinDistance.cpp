#include "LevenshteinDistance.h"
#include <vector>

LevenshteinDistanceC::LevenshteinDistanceC()
{
}

float LevenshteinDistanceC::getDistance(const TCHAR *s1, const TCHAR *s2) const
{
  // Step 1

  int n = _tcslen(s1);
  int m = _tcslen(s2);
  
  if( n == 0 )
    return (float)m;
  if( m == 0 )
    return (float)n;

  // Construct a matrix.
  std::vector< std::vector<int> > matrix(n+1);

  for( int i = 0; i <= n; i++ )
    matrix[i].resize(m+1);


  // Step 2
  for( int i = 0; i <= n; i++ )
    matrix[i][0] = i;

  for( int j = 0; j <= m; j++ )
    matrix[0][j] = j;

  // Step 3

  for( int i = 1; i <= n; i++ )
  {
    const TCHAR s_i = s1[i-1];

    for( int j = 1; j <= m; j++ )
    {
      const TCHAR t_j = s2[j-1];

      int cost;

      // TODO // !!! Why this fucking shit doesn't work on my system? !!!
      //if( wcscmp( &s_i, &t_j ) == 0 )
      //  cost = 0;
      //else
      //  cost = 1;

      std::wstring s1; s1 = s_i;
      std::wstring s2; s2 = t_j;
      if( s1 == s2 )
        cost = 0;
      else
        cost = 1;

      // The cell above plus 1.
      int above = matrix[i-1][j] + 1;
      // The cell to the left plus 1.
      int left  = matrix[i][j-1] + 1;
      // The cell diagonally above and to the left plus the cost.
      int diag  = matrix[i-1][j-1] + cost;

      // Set the cell matrix[si,ti] to the minimum of the three values above.
      int cell = minimum( minimum(above,left), diag );
      matrix[i][j] = cell;
    }
  }

  // The distance is now saved in the matrix at coord [slen,tlen].
  int theLevenshteinDistance = matrix[n][m];

  // Calculate a floating point factor value.
  float distance = 1.0F - ((float)theLevenshteinDistance / maximum(n,m));
  return distance;
}
