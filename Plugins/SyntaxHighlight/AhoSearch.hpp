#ifndef AhoSearch_h__
#define AhoSearch_h__

#include <queue>
#include <map>
#include <vector>
//#include <ctime>

//#include <hash_map>

//#include "MessageCollector.hpp"



// Implementation of the Aho-Corasik algorithm
class AhoSearch
{
private:

  typedef std::map<wchar_t, int> Edges;

  struct SVertex
  {
    Edges edges;
    wchar_t ch;     

    int parent;    // parent vertex
    int link;      // link to the longest suffix

    int out;              // function 'out'
    int length;           // length of a pattern which ends here
    unsigned int param;   // user param 

    SVertex() : ch(0), parent(-1), link(-1), out(-1), param(0), length(0) {}
  };


private:

  std::vector<SVertex> vertices;

  int numberOfWords;

  int lengthOfTheLongestWord;


private:

  int direct(int v, wchar_t c) const
  {
    Edges::const_iterator i = vertices[v].edges.find(c);
    if (i != vertices[v].edges.end())
      return i->second;
    return v > 0 ? -1 : 0;
  }

  int go(int v, wchar_t c) const
  {
    Edges::const_iterator i = vertices[v].edges.find(c);
    if (i != vertices[v].edges.end())
      return i->second;
    return v > 0 ? go(vertices[v].link, c) : 0;
  }

public:
  // type of the result
  struct AhoMatchItem
  {
    int pos;               // index in a string
    int length;            // length of the founded string
    unsigned int param;    // user param of the founded string

    AhoMatchItem() : pos(0), length(0), param(0) {}
    AhoMatchItem(int _pos, int _length, unsigned int _param) : pos(_pos), length(_length), param(_param) {}
  };

  typedef std::vector<AhoMatchItem> AhoMatches;


public:

  // Function adds new string(pattern) to the automat
  void AddString(const wchar_t *str, int length, unsigned int param)
  {
    Edges::iterator edge;
    SVertex vertex;
    int v = 0;

    for (int i = 0; i < length; ++i, ++str)
    {
      if ((edge = vertices[v].edges.find(*str)) != vertices[v].edges.end())
        v = edge->second;
      else // insert new vertex
      {
        vertex.ch = *str;
        vertex.parent = v;
        vertices.push_back(vertex);

        vertices[v].edges.insert(std::make_pair(*str, vertices.size() - 1));
        v = vertices.size() - 1;
      }
    }

    if (0 == vertices[v].length) 
    {
      vertices[v].param = param;
      vertices[v].length = length;
    }

    ++numberOfWords;
    lengthOfTheLongestWord = (lengthOfTheLongestWord >= length) ? lengthOfTheLongestWord : length;
  }

  // Function computes links (must be called when all strings are added)
  void Prepare()
  {
    std::queue<int> q;
    Edges::const_iterator rootEdge = vertices[0].edges.begin();
    Edges::const_iterator edge;
    int v = 0, i, k;

    // fill the root
    while (rootEdge != vertices[0].edges.end())
    {
      q.push(rootEdge->second);
      vertices[rootEdge->second].link = 0;
      ++rootEdge;
    }

    // fill other vertices
    while (!q.empty())
    {
      v = q.front();
      q.pop();

      edge = vertices[v].edges.begin();
      while (edge != vertices[v].edges.end())
      {
        i = edge->second;
        q.push(i);

        k = vertices[v].link;
        while (direct(k, vertices[i].ch) == -1)
          k = vertices[k].link;
        vertices[i].link = direct(k, vertices[i].ch);

        k = vertices[i].link;
        if (vertices[k].length > 0) // this vertex contains match
          vertices[i].out = k; // first vertex with match
        else
          vertices[i].out = vertices[k].out;

        ++edge;
      }
    }
  }

  void Search(const wchar_t *str, int length, AhoMatches &matches, bool allowIntersections = false) const
  {
    int v = 0, u;

    matches.clear();

    for (int i = 0; i < length; ++i)
    {
      v = go(v, *str++);
      if (vertices[v].length > 0) // match
        matches.push_back(AhoMatchItem(i + 1 - vertices[v].length, vertices[v].length, vertices[v].param));

      u = v;
      while ((u = vertices[u].out) > -1) // other matches
        if (vertices[u].parent > -1)
          matches.push_back(AhoMatchItem(i + 1 - vertices[u].length, vertices[u].length, vertices[u].param));
    }

    if (!allowIntersections)
      RemoveIntersections(matches);

    //static wchar_t buff[32];
    //wsprintf(buff, L"Length: %d\n", length);

    //static std::wstring msg;
    //msg.assign(buff);
    //MessageCollector::GetInstance().AddMessage(information, msg);
  }

  int GetNumberOfWords() const 
  {
    return numberOfWords;
  }

  int GetLengthOfTheLongestWord() const
  {
    return lengthOfTheLongestWord;
  }

  AhoSearch() : numberOfWords(0), lengthOfTheLongestWord(0)
  {
    SVertex v;
    v.link = 0;
    v.parent = 0;
    vertices.push_back(v);
  }


public:

  static void RemoveIntersections(AhoMatches &matches)
  {
    if (matches.size() > 1) 
    {      
      AhoSearch::AhoMatches::const_iterator current = matches.begin() + 1, prev = matches.begin();
      while (current != matches.end()) 
      {
        if (prev->pos + prev->length > current->pos)
        {
          current = matches.erase(current);
        }
        else
        {
          ++current;
          ++prev;
        }        
      }
    }
  }

};

#endif // AhoSearch_h__