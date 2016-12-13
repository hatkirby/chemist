#include <yaml-cpp/yaml.h>
#include <iostream>
#include <sstream>
#include <verbly.h>
#include <fstream>
#include <twitter.h>
#include <random>
#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cout << "usage: chemist [configfile]" << std::endl;
    return -1;
  }

  std::string configfile(argv[1]);
  YAML::Node config = YAML::LoadFile(configfile);
  
  twitter::auth auth;
  auth.setConsumerKey(config["consumer_key"].as<std::string>());
  auth.setConsumerSecret(config["consumer_secret"].as<std::string>());
  auth.setAccessKey(config["access_key"].as<std::string>());
  auth.setAccessSecret(config["access_secret"].as<std::string>());
  
  twitter::client client(auth);
  
  std::map<std::string, std::vector<std::string>> groups;
  std::ifstream datafile(config["forms_file"].as<std::string>());
  if (!datafile.is_open())
  {
    std::cout << "Could not find datafile" << std::endl;
    return 1;
  }
  
  bool newgroup = true;
  std::string line;
  std::string curgroup;
  while (getline(datafile, line))
  {
    if (line.back() == '\r')
    {
      line.pop_back();
    }
    
    if (newgroup)
    {
      curgroup = line;
      newgroup = false;
    } else {
      if (line.empty())
      {
        newgroup = true;
      } else {
        groups[curgroup].push_back(line);
      }
    }
  }
  
  std::random_device random_device;
  std::mt19937 random_engine{random_device()};
  
  verbly::data database {config["verbly_datafile"].as<std::string>()};
  for (;;)
  {
    std::cout << "Generating tweet" << std::endl;
    std::string action = "{Main}";
    int tknloc;
    while ((tknloc = action.find("{")) != std::string::npos)
    {
      std::string token = action.substr(tknloc+1, action.find("}")-tknloc-1);
      std::string modifier;
      int modloc;
      if ((modloc = token.find(":")) != std::string::npos)
      {
        modifier = token.substr(modloc+1);
        token = token.substr(0, modloc);
      }
      
      std::string canontkn;
      std::transform(std::begin(token), std::end(token), std::back_inserter(canontkn), [] (char ch) {
        return std::toupper(ch);
      });
      
      std::string result;
      if (canontkn == "NOUN")
      {
        result = database.nouns().is_not_proper().random().limit(1).with_complexity(1).run().front().singular_form();
      } else if (canontkn == "ATTRIBUTE")
      {
        result = database.nouns().random().limit(1).full_hyponym_of(database.nouns().with_wnid(100024264).limit(1).run().front()).run().front().singular_form();
      } else if (canontkn == "ADJECTIVE")
      {
        result = database.adjectives().with_complexity(1).random().limit(1).run().front().base_form();
      } else if (canontkn == "VERBING")
      {
        result = database.verbs().random().limit(1).run().front().ing_form();
      } else if (canontkn == "YEAR")
      {
        std::uniform_int_distribution<int> yeardist(1916,2015);
        int year = yeardist(random_engine);
        result = std::to_string(year);
      } else if (canontkn == "REGION")
      {
        auto hem1 = database.nouns().with_singular_form("eastern hemisphere").limit(1).run().front();
        auto hem2 = database.nouns().with_singular_form("western hemisphere").limit(1).run().front();
        verbly::filter<verbly::noun> region{hem1, hem2};
        region.set_orlogic(true);
        
        result = database.nouns().full_part_holonym_of(region).random().limit(1).run().front().singular_form();
      } else if (canontkn == "FAMOUSNAME")
      {
        auto person = database.nouns().with_singular_form("person").limit(1).run().front();
        auto ptypes = database.nouns().full_hyponym_of({person}).is_class().random().limit(1).run().front();
        result = database.nouns().instance_of({ptypes}).random().limit(1).run().front().singular_form();
      } else if (canontkn == "BODYPART")
      {
        auto bp = database.nouns().with_singular_form("body part").limit(1).run().front();
        result = database.nouns().full_hyponym_of({bp}).with_complexity(1).random().limit(1).run().front().singular_form();
      } else if (canontkn == "\\N")
      {
        result = "\n";
      } else {
        auto group = groups[canontkn];
        std::uniform_int_distribution<int> groupdist(0, group.size()-1);
        int groupind = groupdist(random_engine);
        result = group[groupind];
      }
      
      if (modifier == "indefinite")
      {
        if ((result.length() > 1) && (isupper(result[0])) && (isupper(result[1])))
        {
          result = "an " + result;
        } else if ((result[0] == 'a') || (result[0] == 'e') || (result[0] == 'i') || (result[0] == 'o') || (result[0] == 'u'))
        {
          result = "an " + result;
        } else {
          result = "a " + result;
        }
      }
      
      std::string finalresult;
      if (islower(token[0]))
      {
        std::transform(std::begin(result), std::end(result), std::back_inserter(finalresult), [] (char ch) {
          return std::tolower(ch);
        });
      } else if (isupper(token[0]) && !isupper(token[1]))
      {
        auto words = verbly::split<std::list<std::string>>(result, " ");
        for (auto& word : words)
        {
          word[0] = std::toupper(word[0]);
        }
        
        finalresult = verbly::implode(std::begin(words), std::end(words), " ");
      } else {
        finalresult = result;
      }
      
      action.replace(tknloc, action.find("}")-tknloc+1, finalresult);
    }
    
    action.resize(140);
    
    try
    {
      client.updateStatus(action);
      
      std::cout << "Tweeted!" << std::endl;
    } catch (const twitter::twitter_error& e)
    {
      std::cout << "Twitter error: " << e.what() << std::endl;
    }
    
    std::cout << "Waiting..." << std::endl;
    
    std::this_thread::sleep_for(std::chrono::hours(1));
  }
}
