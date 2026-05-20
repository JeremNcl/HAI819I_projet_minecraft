#ifndef OBJLOADER_H
#define OBJLOADER_H
#include <string>

bool loadOFF( const std::string & filename ,
              std::vector< glm::vec3 > & vertices ,
              std::vector< unsigned short > & indices,
              std::vector< std::vector<unsigned short > > & triangles) ;

bool loadOFF( const std::string & filename ,
			  std::vector< glm::vec3 > & vertices ,
			  std::vector< unsigned int > & indices,
			  std::vector< std::vector<unsigned int > > & triangles) ;

#endif
