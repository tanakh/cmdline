#include "cmdline.h"

#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
  cmdline::parser a;
  a.add<string>("host", 'h', "ホスト名", true);
  a.add<int>("port", 'p', "ポート番号", false, 80);
  a.add("help", 0, "ヘルプを表示します");
  a.footer("ファイル名 ...");

  if (argc==1){
    cerr<<a.usage()<<endl;
    return 0;
  }

  if (!a.parse(argc, argv)){
    cerr<<a.error()<<endl<<a.usage();
    return 0;
  }

  if (a.exist("help")){
    cerr<<a.usage();
    return 0;
  }

  cout<<a.get<string>("host")<<":"<<a.get<int>("port")<<endl;

  for (size_t i=0; i<a.rest().size(); i++)
    cout<<"- "<<a.rest()[i]<<endl;

  return 0;
}
