#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>

#include <glm/glm.hpp>

#include "objectLoader.hpp"


// Very, VERY simple OBJ loader.
// Here is a short list of features a real function would provide : 
// - Binary files. Reading a model should be just a few memcpy's away, not parsing a file at runtime. In short : OBJ is not very great.
// - Animations & bones (includes bones weights)
// - Multiple UVs
// - All attributes should be optional, not "forced"
// - More stable. Change a line in the OBJ file and it crashes.
// - More secure. Change another line and you can inject code.
// - Loading from memory, stream, etc

bool loadOFF( const std::string & filename ,
              std::vector< glm::vec3 > & vertices ,
              std::vector< unsigned short > & indices,
              std::vector< std::vector<unsigned short > > & triangles )
{
    bool convertToTriangles = true;
    bool randomize = false;

    std::ifstream myfile;
    myfile.open(filename.c_str());
    if (!myfile.is_open())
    {
        std::cout << filename << " cannot be opened" << std::endl;
        return false;
    }

    std::string magic_s;

    myfile >> magic_s;

    if( magic_s != "OFF" )
    {
        std::cout << magic_s << " != OFF :   We handle ONLY *.off files." << std::endl;
        myfile.close();
        return false;
    }

    int n_vertices , n_faces , dummy_int;
    myfile >> n_vertices >> n_faces >> dummy_int;

    vertices.resize(n_vertices);

    for( int v = 0 ; v < n_vertices ; ++v )
    {
        glm::vec3 vertex;
        myfile >> vertex.x >> vertex.y >> vertex.z;
        if( std::isnan(vertex.x) )
            vertex.x = 0.0;
        if( std::isnan(vertex.y) )
            vertex.y = 0.0;
        if( std::isnan(vertex.z) )
            vertex.z = 0.0;
        vertices[v] = vertex;
    }


    for( int f = 0 ; f < n_faces ; ++f )
    {
        int n_vertices_on_face;
        myfile >> n_vertices_on_face;
        if( n_vertices_on_face == 3 )
        {
            unsigned short _v1 , _v2 , _v3;
            std::vector< unsigned short > _v;
            myfile >> _v1 >> _v2 >> _v3;
            _v.push_back( _v1 );
            _v.push_back( _v2 );
            _v.push_back( _v3 );
            triangles.push_back( _v );
            indices.push_back( _v1 );
            indices.push_back( _v2 );
            indices.push_back( _v3 );

        }
        else if( n_vertices_on_face > 3 )
        {
            std::vector< unsigned short > vhandles;
            vhandles.resize(n_vertices_on_face);
            for( int i=0 ; i < n_vertices_on_face ; ++i )
                myfile >> vhandles[i];

            if( convertToTriangles )
            {
                unsigned short k=(randomize)?(rand()%vhandles.size()):0;
                for (unsigned short i=0;i<vhandles.size()-2;++i)
                {
                    std::vector< unsigned short > tri(3);
                    tri[0]=vhandles[(k+0)%vhandles.size()];
                    tri[1]=vhandles[(k+i+1)%vhandles.size()];
                    tri[2]=vhandles[(k+i+2)%vhandles.size()];
                    triangles.push_back(tri);
                    indices.push_back(tri[0]);
                    indices.push_back(tri[1]);
                    indices.push_back(tri[2]);
                }
            }
            else
            {
                //careful not triangles
                triangles.push_back(vhandles);
                for( int i=0 ; i < vhandles.size() ; ++i )
                    indices.push_back(vhandles[i]);
            }
        }
        else
        {
            std::cout << "OFFIO::open error : Face number " << f << " has " << n_vertices_on_face << " vertices" << std::endl;
            myfile.close();
            return false;
        }
    }

    myfile.close();
    return true;
}

bool loadOFF( const std::string & filename ,
              std::vector< glm::vec3 > & vertices ,
              std::vector< unsigned int > & indices,
              std::vector< std::vector<unsigned int > > & triangles )
{
    bool convertToTriangles = true;
    bool randomize = false;

    std::ifstream myfile;
    myfile.open(filename.c_str());
    if (!myfile.is_open())
    {
        std::cout << filename << " cannot be opened" << std::endl;
        return false;
    }

    std::string magic_s;

    myfile >> magic_s;

    if( magic_s != "OFF" )
    {
        std::cout << magic_s << " != OFF :   We handle ONLY *.off files." << std::endl;
        myfile.close();
        return false;
    }

    int n_vertices , n_faces , dummy_int;
    myfile >> n_vertices >> n_faces >> dummy_int;

    vertices.resize(n_vertices);

    for( int v = 0 ; v < n_vertices ; ++v )
    {
        glm::vec3 vertex;
        myfile >> vertex.x >> vertex.y >> vertex.z;
        if( std::isnan(vertex.x) )
            vertex.x = 0.0;
        if( std::isnan(vertex.y) )
            vertex.y = 0.0;
        if( std::isnan(vertex.z) )
            vertex.z = 0.0;
        vertices[v] = vertex;
    }

    for( int f = 0 ; f < n_faces ; ++f )
    {
        int n_vertices_on_face;
        myfile >> n_vertices_on_face;
        if( n_vertices_on_face == 3 )
        {
            unsigned int _v1 , _v2 , _v3;
            std::vector< unsigned int > _v;
            myfile >> _v1 >> _v2 >> _v3;
            _v.push_back( _v1 );
            _v.push_back( _v2 );
            _v.push_back( _v3 );
            triangles.push_back( _v );
            indices.push_back( _v1 );
            indices.push_back( _v2 );
            indices.push_back( _v3 );

        }
        else if( n_vertices_on_face > 3 )
        {
            std::vector< unsigned int > vhandles;
            vhandles.resize(n_vertices_on_face);
            for( int i=0 ; i < n_vertices_on_face ; ++i )
                myfile >> vhandles[i];

            if( convertToTriangles )
            {
                unsigned int k=(randomize)?(rand()%vhandles.size()):0;
                for (unsigned int i=0;i<vhandles.size()-2;++i)
                {
                    std::vector< unsigned int > tri(3);
                    tri[0]=vhandles[(k+0)%vhandles.size()];
                    tri[1]=vhandles[(k+i+1)%vhandles.size()];
                    tri[2]=vhandles[(k+i+2)%vhandles.size()];
                    triangles.push_back(tri);
                    indices.push_back(tri[0]);
                    indices.push_back(tri[1]);
                    indices.push_back(tri[2]);
                }
            }
            else
            {
                triangles.push_back(vhandles);
                for( int i=0 ; i < vhandles.size() ; ++i )
                    indices.push_back(vhandles[i]);
            }
        }
        else
        {
            std::cout << "OFFIO::open error : Face number " << f << " has " << n_vertices_on_face << " vertices" << std::endl;
            myfile.close();
            return false;
        }
    }

    myfile.close();
    return true;
}