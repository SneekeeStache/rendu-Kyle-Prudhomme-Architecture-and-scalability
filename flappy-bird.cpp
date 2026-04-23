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


struct bird_data {
  float position_y = 9.0f;
  float velocity = 0.0f;
  int box_collision_shape_top = 0;
  int box_collision_shap_bottom = 0;
  int box_collision_shap_side_left = 10;
  int box_collision_shap_side_right = 10 + 2 - 1;
  int death_status = 0;               // 0 = alive, 1 = dead
  float spawn_timer = 0.0f;
};

struct  hud_parameters {
  int left_padding = 0; // left padding
  int right_padding = 0; // right padding
};

struct pipe_data {
  std::vector<float> position_x;
  std::vector<int> gap_top;
  std::vector<int> scored_flag;
};



void load_best_score(unsigned long long& game_best_score) {
  std::ifstream fin("best-score.txt");
  if (fin) {
    fin >> game_best_score;
    if (!fin)
      game_best_score = 0; // reset if read failed
  }
  fin.close();
}

void save_best_score(unsigned long long game_best_score) {
  std::ofstream fout("best-score.txt", std::ios::trunc);
  if (fout)
    fout << game_best_score; // write best score
  fout.close();
}

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

  bird_data* bird = new bird_data;
  unsigned long long game_current_score = 0;  // current score
  unsigned long long game_best_score = 0; // best score
  hud_parameters* hud = new hud_parameters;
  pipe_data* pipe = new pipe_data;

  // rng
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> gap_position(2, 20 - 6 - 2); // gap position

  INPUT_RECORD rec;
  DWORD ne = 0;

  // load best score
  load_best_score(game_best_score);
   // close the file

  auto prev = std::chrono::steady_clock::now();

  // main loop
  while (bird->death_status == 0) {
    // delta time
    auto now = std::chrono::steady_clock::now();
    float delta_time = std::chrono::duration<float>(now - prev).count();
    prev = now;
    if (delta_time > 0.1f)
      delta_time = 0.1f; // clamp delta time

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
        KEY_EVENT_RECORD key_event_recorded = rec.Event.KeyEvent;
        if (key_event_recorded.bKeyDown == TRUE) {
          if (key_event_recorded.wVirtualKeyCode == VK_RETURN) {
            bird->velocity = -14.0f;
          } // end if enter
        } // end if key down
      } // end if key event
    } // end for each event

    bird->velocity = bird->velocity + 42.0f * delta_time;
    bird->position_y = bird->position_y + bird->velocity * delta_time;

    bird->spawn_timer = bird->spawn_timer + delta_time;
    if (bird->spawn_timer >= 1.4f) {
      bird->spawn_timer = bird->spawn_timer - 1.4f;
      pipe->position_x.push_back(50.0f);
      pipe->gap_top.push_back(gap_position(rng));
      pipe->scored_flag.push_back(0);
    }

    for (int i = 0; i < (int)pipe->position_x.size(); i++) // loop over all pipes
    {
      pipe->position_x[i] = pipe->position_x[i] - 18.0f * delta_time; // move pipe left

      int pipe_right = (int)std::floor(pipe->position_x[i]) + 6 - 1;
      if (pipe->scored_flag[i] == 0 && pipe_right < 10) {
        pipe->scored_flag[i] = 1;
        game_current_score = game_current_score + 1;
        if (game_current_score > game_best_score)
          game_best_score = game_current_score;
      }
    }

    for (int i = (int)pipe->position_x.size() - 1; i >= 0; i--) {
      if (pipe->position_x[i] + 6.0f < 0.0f) {
        pipe->position_x.erase(pipe->position_x.begin() + i);
        pipe->gap_top.erase(pipe->gap_top.begin() + i);
        pipe->scored_flag.erase(pipe->scored_flag.begin() + i);
      }
    }

    // collision
    bird->box_collision_shape_top = (int)std::floor(bird->position_y);
    bird->box_collision_shap_bottom = bird->box_collision_shape_top + 2 - 1;
    bird->box_collision_shap_side_left = 10;         // same every frame
    bird->box_collision_shap_side_right = 10 + 2 - 1; // same every frame
    // check wall
    if (bird->box_collision_shape_top < 0 || bird->box_collision_shap_bottom >= 20) {
      bird->death_status = 1;
    }

    if (!bird->death_status) {
      for (int i = 0; i < (int)pipe->position_x.size(); i++) {
        int pl = (int)std::floor(pipe->position_x[i]);
        int pr = pl + 6 - 1;

        if (bird->box_collision_shap_side_right >= pl && bird->box_collision_shap_side_left <= pr) {
          for (int y = bird->box_collision_shape_top; y <= bird->box_collision_shap_bottom; y++) {
            if (y < pipe->gap_top[i] || y >= pipe->gap_top[i] + 6) {
              bird->death_status = 1;
              break;
            }
          }
        }

        if (bird->death_status != 0)
          break;
      }
    }

    if (bird->death_status != 0)
      break;

    std::vector<std::string> frame(20, std::string(50, ' '));

    for (int i = 0; i < (int)pipe->position_x.size(); i++) {
      int pl = (int)std::floor(pipe->position_x[i]);
      for (int dx = 0; dx < 6; dx++) {
        int x = pl + dx;
        if (x < 0 || x >= 50)
          continue;
        for (int y = 0; y < 20; y++) {
          if (!(y >= pipe->gap_top[i] && y < pipe->gap_top[i] + 6)) {
            frame[y][x] = 'P';
          }
        }
      }
    }

    for (int dy = 0; dy < 2; dy++) {
      int y = bird->box_collision_shape_top + dy;
      if (y < 0 || y >= 20)
        continue;
      for (int dx = 0; dx < 2; dx++) {
        int x = 10 + dx;
        if (x >= 0 && x < 50)
          frame[y][x] = 'B';
      }
    }

    std::string scoreText =
        "Score: " + std::to_string(game_current_score) + "   Best: " + std::to_string(game_best_score);
    if (scoreText.size() > 50)
      scoreText = scoreText.substr(0, 50);
    hud->left_padding = (int)((50 - (int)scoreText.size()) / 2);
    hud->right_padding = 50 - hud->left_padding - (int)scoreText.size();

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
    std::cout << "|" << std::string(hud->left_padding, ' ') << scoreText
              << std::string(hud->right_padding, ' ') << "|\n";
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
  save_best_score(game_best_score);

  // restore original console mode
  SetConsoleMode(input_console, m);
  return 0;
}