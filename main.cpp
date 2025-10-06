#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

using namespace std;

struct ProblemStatus {
    bool solved = false;
    int wrong_attempts = 0;
    int solve_time = 0;
    vector<pair<string, int>> frozen_subs;
    bool is_frozen = false;
    bool was_solved_before_freeze = false;
};

struct Submission {
    string team_name;
    string problem_name;
    string status;
    int time;
};

struct Team {
    string name;
    map<char, ProblemStatus> problems;
    vector<Submission> all_submissions;
    
    int solved_count = 0;
    int penalty_time = 0;
    vector<int> solve_times;
    int ranking = 0;
    
    void calculate_stats(int problem_count) {
        solved_count = 0;
        penalty_time = 0;
        solve_times.clear();
        
        for (char p = 'A'; p < 'A' + problem_count; p++) {
            ProblemStatus& ps = problems[p];
            if (ps.solved && !ps.is_frozen) {
                solved_count++;
                penalty_time += ps.solve_time + 20 * ps.wrong_attempts;
                solve_times.push_back(ps.solve_time);
            }
        }
        sort(solve_times.rbegin(), solve_times.rend());
    }
    
    bool operator<(const Team& other) const {
        if (solved_count != other.solved_count) {
            return solved_count > other.solved_count;
        }
        if (penalty_time != other.penalty_time) {
            return penalty_time < other.penalty_time;
        }
        size_t min_size = min(solve_times.size(), other.solve_times.size());
        for (size_t i = 0; i < min_size; i++) {
            if (solve_times[i] != other.solve_times[i]) {
                return solve_times[i] < other.solve_times[i];
            }
        }
        return name < other.name;
    }
};

class ICPCSystem {
private:
    map<string, Team> teams;
    vector<string> team_names;
    bool competition_started = false;
    bool is_frozen = false;
    int duration_time = 0;
    int problem_count = 0;
    
    void update_all_stats() {
        for (auto& p : teams) {
            p.second.calculate_stats(problem_count);
        }
    }
    
    vector<Team*> get_sorted_teams() {
        vector<Team*> sorted_teams;
        for (const string& name : team_names) {
            sorted_teams.push_back(&teams[name]);
        }
        sort(sorted_teams.begin(), sorted_teams.end(), 
             [](Team* a, Team* b) { return *a < *b; });
        return sorted_teams;
    }
    
    void assign_rankings() {
        auto sorted_teams = get_sorted_teams();
        for (size_t i = 0; i < sorted_teams.size(); i++) {
            sorted_teams[i]->ranking = i + 1;
        }
    }
    
    void print_scoreboard() {
        auto sorted_teams = get_sorted_teams();
        
        for (Team* team : sorted_teams) {
            cout << team->name << " " << team->ranking << " " 
                 << team->solved_count << " " << team->penalty_time;
            
            for (char p = 'A'; p < 'A' + problem_count; p++) {
                cout << " ";
                ProblemStatus& ps = team->problems[p];
                
                if (ps.is_frozen) {
                    int frozen_count = ps.frozen_subs.size();
                    if (ps.wrong_attempts > 0) {
                        cout << "-" << ps.wrong_attempts << "/" << frozen_count;
                    } else {
                        cout << "0/" << frozen_count;
                    }
                } else if (ps.solved) {
                    if (ps.wrong_attempts > 0) {
                        cout << "+" << ps.wrong_attempts;
                    } else {
                        cout << "+";
                    }
                } else {
                    if (ps.wrong_attempts > 0) {
                        cout << "-" << ps.wrong_attempts;
                    } else {
                        cout << ".";
                    }
                }
            }
            cout << "\n";
        }
    }
    
public:
    void add_team(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.find(team_name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        
        Team team;
        team.name = team_name;
        teams[team_name] = team;
        team_names.push_back(team_name);
        cout << "[Info]Add successfully.\n";
    }
    
    void start_competition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        
        competition_started = true;
        duration_time = duration;
        problem_count = problems;
        
        for (auto& p : teams) {
            for (char prob = 'A'; prob < 'A' + problem_count; prob++) {
                p.second.problems[prob] = ProblemStatus();
            }
        }
        
        vector<string> sorted = team_names;
        sort(sorted.begin(), sorted.end());
        for (size_t i = 0; i < sorted.size(); i++) {
            teams[sorted[i]].ranking = i + 1;
        }
        
        cout << "[Info]Competition starts.\n";
    }
    
    void submit(const string& problem_name, const string& team_name, 
                const string& status, int time) {
        Team& team = teams[team_name];
        char problem = problem_name[0];
        ProblemStatus& ps = team.problems[problem];
        
        Submission sub = {team_name, problem_name, status, time};
        team.all_submissions.push_back(sub);
        
        if (is_frozen) {
            if (!ps.solved && !ps.was_solved_before_freeze) {
                ps.frozen_subs.push_back({status, time});
                ps.is_frozen = true;
            }
        } else {
            if (!ps.solved) {
                if (status == "Accepted") {
                    ps.solved = true;
                    ps.solve_time = time;
                } else {
                    ps.wrong_attempts++;
                }
            }
        }
    }
    
    void flush() {
        update_all_stats();
        assign_rankings();
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze() {
        if (is_frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        
        for (auto& p : teams) {
            for (char prob = 'A'; prob < 'A' + problem_count; prob++) {
                if (p.second.problems[prob].solved) {
                    p.second.problems[prob].was_solved_before_freeze = true;
                }
            }
        }
        
        is_frozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void scroll() {
        if (!is_frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        update_all_stats();
        assign_rankings();
        print_scoreboard();
        
        // Process all frozen problems in order
        while (true) {
            // Find lowest ranked team with frozen problems
            Team* target_team = nullptr;
            int lowest_rank = 0;
            
            for (const string& name : team_names) {
                Team& t = teams[name];
                bool has_frozen = false;
                for (char p = 'A'; p < 'A' + problem_count; p++) {
                    if (t.problems[p].is_frozen) {
                        has_frozen = true;
                        break;
                    }
                }
                if (has_frozen && t.ranking > lowest_rank) {
                    lowest_rank = t.ranking;
                    target_team = &t;
                }
            }
            
            if (!target_team) break;
            
            // Find smallest frozen problem
            char unfreeze_prob = 0;
            for (char p = 'A'; p < 'A' + problem_count; p++) {
                if (target_team->problems[p].is_frozen) {
                    unfreeze_prob = p;
                    break;
                }
            }
            
            int old_rank = target_team->ranking;
            
            // Process frozen submissions for this problem
            ProblemStatus& ps = target_team->problems[unfreeze_prob];
            for (const auto& sub : ps.frozen_subs) {
                if (!ps.solved) {
                    if (sub.first == "Accepted") {
                        ps.solved = true;
                        ps.solve_time = sub.second;
                    } else {
                        ps.wrong_attempts++;
                    }
                }
            }
            ps.is_frozen = false;
            ps.frozen_subs.clear();
            
            // Only recalculate stats for the target team
            target_team->calculate_stats(problem_count);
            
            // Reassign all rankings (necessary for correctness)
            assign_rankings();
            
            // Check if ranking improved and output if so
            if (target_team->ranking < old_rank) {
                // Find the team that is now at old_rank or closest below target
                string replaced_name = "";
                for (const string& name : team_names) {
                    if (teams[name].ranking == target_team->ranking + 1) {
                        replaced_name = name;
                        break;
                    }
                }
                
                cout << target_team->name << " " << replaced_name << " " 
                     << target_team->solved_count << " " << target_team->penalty_time << "\n";
            }
        }
        
        print_scoreboard();
        is_frozen = false;
    }
    
    void query_ranking(const string& team_name) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        if (is_frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        cout << team_name << " NOW AT RANKING " << teams[team_name].ranking << "\n";
    }
    
    void query_submission(const string& team_name, const string& problem_filter, 
                         const string& status_filter) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        Team& team = teams[team_name];
        Submission* result = nullptr;
        
        for (int i = team.all_submissions.size() - 1; i >= 0; i--) {
            const Submission& sub = team.all_submissions[i];
            bool prob_match = (problem_filter == "ALL" || sub.problem_name == problem_filter);
            bool stat_match = (status_filter == "ALL" || sub.status == status_filter);
            
            if (prob_match && stat_match) {
                result = const_cast<Submission*>(&sub);
                break;
            }
        }
        
        if (!result) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << result->team_name << " " << result->problem_name << " " 
                 << result->status << " " << result->time << "\n";
        }
    }
    
    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    
    ICPCSystem system;
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            system.add_team(team_name);
        }
        else if (command == "START") {
            string dur_str, prob_str;
            int duration, problems;
            iss >> dur_str >> duration >> prob_str >> problems;
            system.start_competition(duration, problems);
        }
        else if (command == "SUBMIT") {
            string problem, by, team, with, status, at;
            int time;
            iss >> problem >> by >> team >> with >> status >> at >> time;
            system.submit(problem, team, status, time);
        }
        else if (command == "FLUSH") {
            system.flush();
        }
        else if (command == "FREEZE") {
            system.freeze();
        }
        else if (command == "SCROLL") {
            system.scroll();
        }
        else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.query_ranking(team_name);
        }
        else if (command == "QUERY_SUBMISSION") {
            string team_name, where, problem_part, and_part, status_part;
            iss >> team_name >> where >> problem_part >> and_part >> status_part;
            
            size_t eq_pos = problem_part.find('=');
            string problem_filter = problem_part.substr(eq_pos + 1);
            
            eq_pos = status_part.find('=');
            string status_filter = status_part.substr(eq_pos + 1);
            
            system.query_submission(team_name, problem_filter, status_filter);
        }
        else if (command == "END") {
            system.end();
            break;
        }
    }
    
    return 0;
}
