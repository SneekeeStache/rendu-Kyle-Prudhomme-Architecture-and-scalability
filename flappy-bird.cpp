//explication du code:
/*
    le code est un jeu inspirer de flappy bird qui se joue dans une console,
    initialise ses variable a l'execution 
    utilise une boucle while pour servire d'equivalent tick avec delta time afin de gerer
        - la detection d'input
        - les collisions
        - le rendering
        - la position
        - l'etat du joueur
    et enregistre le meilleur score dans un fichier text

//défauts d’implémentation
/*
    - God Object:
        tout le code s'execute dans main sur un seul script
    - variable pas clair:
        toutes les variable du code utilise des nom de variable en abreviation ou qui n'ont pas de sens
    - ancien code inutiliser toujours present meme si commenter
    - probleme de lisibilité due a des if dans des if dans des if
    
*/


// TODO: clean this up later

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>


int main() {
  HANDLE input_console = GetStdHandle(STD_INPUT_HANDLE);   // input
  HANDLE output_console = GetStdHandle(STD_OUTPUT_HANDLE); // output
  if (input_console == INVALID_HANDLE_VALUE) {
    std::cerr << "error" << std::endl;
    return 1;
  }
  if (output_console == INVALID_HANDLE_VALUE) {
    std::cerr << "error" << std::endl;
    return 1;
  }

  // use words for console io
  DWORD m = 0;
  DWORD m2 = 0;
  DWORD m3 = 0;
  if (!GetConsoleMode(input_console, &m)) {
    std::cerr << "error" << std::endl;
    return 1;
  }
  m2 = m;
  m2 &= ~ENABLE_LINE_INPUT;
  m2 &= ~ENABLE_ECHO_INPUT;
  if (!SetConsoleMode(input_console, m2)) {
    std::cerr << "error" << std::endl;
    return 1;
  }
  if (GetConsoleMode(output_console, &m3))
    SetConsoleMode(output_console, m3 | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

  // bird
  float bird_position_y = 9.0f;          
  float bird_velocity = 0.0f;           
  int bird_box_collision_shape_top = 0;                
  int bird_box_collision_shap_bottom = 0;               
  int bird_box_collision_shap_side_left = 10;              
  int bird_box_collision_shap_side_right = 10 + 2 - 1;  
  int dead = 0;               // 0 = alive, 1 = dead
  float spawn_timer = 0.0f;
  unsigned long long current_score = 0;  // current score
  unsigned long long best_score = 0; // best score
  // hud padding
  int lp = 0; // left padding
  int rp = 0; // right padding
  // pipe data
  std::vector<float> px; // x positions
  std::vector<int> pg;   // gap tops
  std::vector<int> ps;   // scored flag (0 or 1)

  // rng
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> d(2, 20 - 6 - 2); // gap position

  INPUT_RECORD rec;
  DWORD ne = 0;

  // int debug = 1;
  // if (debug) std::cout << "debug: game starting" << std::endl;

  // load best score
  std::ifstream fin("best-score.txt");
  if (fin) {
    fin >> best_score;
    if (!fin)
      best_score = 0; // reset if read failed
  }
  fin.close(); // close the file

  auto prev = std::chrono::steady_clock::now();

  // main loop
  while (dead == 0) {
    // delta time
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - prev).count();
    prev = now;
    if (dt > 0.1f)
      dt = 0.1f; // clamp delta time

    // read input events
    DWORD nEvents = 0;
    if (!GetNumberOfConsoleInputEvents(input_console, &nEvents)) {
      SetConsoleMode(input_console, m);
      return 1;
    }

    for (DWORD i = 0; i < nEvents; ++i) // loop through events
    {
      if (!ReadConsoleInput(input_console, &rec, 1, &ne)) {
        std::cerr << "Failed to read console input." << std::endl;
        SetConsoleMode(input_console, m);
        return 1;
      } // end if ReadConsoleInput

      if (rec.EventType == KEY_EVENT) {
        KEY_EVENT_RECORD k = rec.Event.KeyEvent;
        if (k.bKeyDown == TRUE) {
          if (k.wVirtualKeyCode == VK_RETURN) {
            bird_velocity = -14.0f;
          } // end if enter
        } // end if key down
      } // end if key event
    } // end for each event

    bird_velocity = bird_velocity + 42.0f * dt;
    bird_position_y = bird_position_y + bird_velocity * dt;

    spawn_timer = spawn_timer + dt;
    if (spawn_timer >= 1.4f) {
      spawn_timer = spawn_timer - 1.4f;
      px.push_back(50.0f);
      pg.push_back(d(rng));
      ps.push_back(0);
    }

    for (int i = 0; i < (int)px.size(); i++) // loop over all pipes
    {
      px[i] = px[i] - 18.0f * dt; // move pipe left

      int pipeRight = (int)std::floor(px[i]) + 6 - 1;
      if (ps[i] == 0 && pipeRight < 10) {
        ps[i] = 1;
        current_score = current_score + 1;
        if (current_score > best_score)
          best_score = current_score;
      }
    }

    for (int i = (int)px.size() - 1; i >= 0; i--) {
      if (px[i] + 6.0f < 0.0f) {
        px.erase(px.begin() + i);
        pg.erase(pg.begin() + i);
        ps.erase(ps.begin() + i);
      }
    }

    // collision
    bird_box_collision_shape_top = (int)std::floor(bird_position_y);
    bird_box_collision_shap_bottom = bird_box_collision_shape_top + 2 - 1;
    bird_box_collision_shap_side_left = 10;         // same every frame
    bird_box_collision_shap_side_right = 10 + 2 - 1; // same every frame
    // check wall
    if (bird_box_collision_shape_top < 0 || bird_box_collision_shap_bottom >= 20) {
      dead = 1;
    }

    if (!dead) {
      for (int i = 0; i < (int)px.size(); i++) {
        int pl = (int)std::floor(px[i]);
        int pr = pl + 6 - 1;

        if (bird_box_collision_shap_side_right >= pl && bird_box_collision_shap_side_left <= pr) {
          for (int y = bird_box_collision_shape_top; y <= bird_box_collision_shap_bottom; y++) {
            if (y < pg[i] || y >= pg[i] + 6) {
              dead = 1;
              break;
            }
          }
        }

        if (dead != 0)
          break;
      }
    }

    if (dead != 0)
      break;

    std::vector<std::string> frame(20, std::string(50, ' '));

    for (int i = 0; i < (int)px.size(); i++) {
      int pl = (int)std::floor(px[i]);
      for (int dx = 0; dx < 6; dx++) {
        int x = pl + dx;
        if (x < 0 || x >= 50)
          continue;
        for (int y = 0; y < 20; y++) {
          if (!(y >= pg[i] && y < pg[i] + 6)) {
            frame[y][x] = 'P';
          }
        }
      }
    }

    for (int dy = 0; dy < 2; dy++) {
      int y = bird_box_collision_shape_top + dy;
      if (y < 0 || y >= 20)
        continue;
      for (int dx = 0; dx < 2; dx++) {
        int x = 10 + dx;
        if (x >= 0 && x < 50)
          frame[y][x] = 'B';
      }
    }

    std::string scoreText =
        "Score: " + std::to_string(current_score) + "   Best: " + std::to_string(best_score);
    if (scoreText.size() > 50)
      scoreText = scoreText.substr(0, 50);
    lp = (int)((50 - (int)scoreText.size()) / 2);
    rp = 50 - lp - (int)scoreText.size();

    std::cout << "\x1b[2J\x1b[H";
    std::cout << "+" << std::string(50, '-') << "+" << "\n";
    for (int y = 0; y < 20; y++) {
      std::cout << "|";
      for (int x = 0; x < 50; x++) {
        char c = frame[y][x];
        if (c == 'P') {
          std::cout << "\x1b[32mP\x1b[0m";
        } else if (c == 'B') {
          std::cout << "\x1b[33mB\x1b[0m";
        } else {
          std::cout << ' ';
        }
      }
      std::cout << "|\n";
    }
    std::cout << "+" << std::string(50, '-') << "+" << "\n";
    std::cout << "+" << std::string(50, '-') << "+" << "\n";
    std::cout << "|" << std::string(lp, ' ') << scoreText
              << std::string(rp, ' ') << "|\n";
    std::cout << "+" << std::string(50, '-') << "+" << "\n";
    std::cout.flush();

    float ft =
        std::chrono::duration<float>(std::chrono::steady_clock::now() - now)
            .count();
    if (ft < 1.0f / 30.0f) {
      std::this_thread::sleep_for(
          std::chrono::duration<float>(1.0f / 30.0f - ft));
    }
  }

  // save best score to file
  std::ofstream fout("best-score.txt", std::ios::trunc);
  if (fout)
    fout << best_score; // write best score
  fout.close();  // close the file

  // restore original console mode
  SetConsoleMode(input_console, m);
  return 0;
}