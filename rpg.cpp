#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <memory>
using namespace std;

class RandomEngine
{
    mt19937 rng{random_device{}()};

public:
    int get(int a, int b) { return uniform_int_distribution<>(a, b)(rng); }
};

class ICombat
{
public:
    virtual ~ICombat() = default;
    virtual int attack(RandomEngine &) = 0;
    virtual void takeDamage(int d) = 0;
    virtual bool alive() const = 0;
    virtual void showStatus(string tag = "ENEMY") const = 0;
    virtual void save(ofstream &f) const = 0;
    virtual void load(ifstream &f) = 0;
    virtual int getTypeID() const = 0;
};

template <class T = int>
class Hero : public ICombat
{
    string name;
    int lv = 1, xp = 0, en = 0, money = 100;
    T hp, maxHp, atk;

public:
    Hero(string n = "", T h = 0, T a = 0) : name(n), hp(h), maxHp(h), atk(a) {}
    string getName() const { return name; }
    int getLevel() const { return lv; }
    bool alive() const override { return hp > 0; }
    int needXP() const { return 100 + (lv - 1) * 50; }
    int getTypeID() const override { return 1; }

    void showStatus(string tag = "Hero") const override
    {
        cout << left << setw(9) << tag << setw(14) << name
             << "Lv." << setw(2) << lv
             << " HP " << setw(3) << hp << "/" << setw(3) << maxHp
             << " ATK" << setw(3) << atk
             << " EN" << setw(3) << en
             << " $" << money << "\n";
    }

    int attack(RandomEngine &r) override
    {
        int d = r.get(atk * 70 / 100, atk * 130 / 100);
        if (r.get(1, 100) <= 10)
        {
            d *= 2;
            cout << "CRITICAL !! \n";
        }
        return d;
    }

    void takeDamage(int d) override { hp = max(0, hp - d); }

    void charge(RandomEngine &r)
    {
        int n = r.get(10, 30);
        en = min(100, en + n);
        cout << name << " Charges +" << n << " EN\n";
    }

    bool skill(RandomEngine &r, ICombat &enemy)
    {
        if (en < 40)
        {
            cout << "Not enough Energy!\n";
            return false;
        }
        en -= 40;
        int d = attack(r) * 2;
        enemy.takeDamage(d);
        cout << name << " uses SKILL -> -" << d << " HP\n";
        return true;
    }

    void heal(RandomEngine &r)
    {
        if (money < 30)
        {
            cout << "Not enough money!\n";
            return;
        }
        int overMaxHP = maxHp * 120 / 100;
        if (hp >= overMaxHP)
        {
            cout << "HP is already at maximum over-heal capacity (120%)!\n";
            return;
        }
        money -= 30;
        int pct = 50;
        int roll = r.get(1, 100);
        if (roll <= 80) pct = r.get(50, 99);
        else if (roll <= 95) pct = 100;
        else pct = r.get(101, 120);

        int amount = maxHp * pct / 100;
        hp = min(overMaxHP, hp + amount);
        if (pct > 100) cout << "CRITICAL HEAL! OVER-HEAL " << pct << "% (+" << amount << " HP)! -$30\n";
        else cout << "Healed " << pct << "% (+" << amount << " HP)! -$30\n";
    }

    void addMoney(int n) { money += n; }
    void addXP(int n)
    {
        xp += n;
        while (lv < 10 && xp >= needXP())
        {
            xp -= needXP();
            lv++;
            maxHp += 20;
            hp = maxHp;
            atk += 5;
            en = 0;
            cout << "LEVEL UP! Now Lv." << lv << "\n";
        }
        if (lv == 10) xp = 0;
    }

    void save(ofstream &f) const override
    {
        f << name << "\n";
        f << lv << ' ' << xp << ' ' << en << ' ' << money << ' ' << hp << ' ' << maxHp << ' ' << atk << '\n';
    }

    void load(ifstream &f) override
    {
        f >> name;
        f >> lv >> xp >> en >> money >> hp >> maxHp >> atk;
    }
};

// ==== Global Function: Quit Game ====
void quitGame(const Hero<int> *player = nullptr, const ICombat *enemy = nullptr)
{
    if (player && player->getName() != "")
    {
        ofstream f("save.txt");
        player->save(f);
        if (enemy && enemy->alive())
        {
            f << enemy->getTypeID() << '\n';
            enemy->save(f);
        }
        else
        {
            f << 0 << '\n';
        }
        cout << "\nGame saved automatically.\n";
    }
    cout << "Exiting game. Goodbye!\n";
    exit(0);
}

// ==== BOSS 1: Redeye ====
class Redeye : public ICombat
{
    int hp, maxHp, atk, en = 0;

public:
    Redeye(int lv = 3) : hp(250 + lv * 15), maxHp(250 + lv * 15), atk(30 + lv * 5) {}
    bool alive() const override { return hp > 0; }
    int getTypeID() const override { return 2; }

    void showStatus(string tag = "BOSS") const override
    {
        cout << left << setw(9) << tag << setw(14) << "Redeye"
             << "HP " << setw(3) << hp << "/" << setw(4) << maxHp
             << " ATK" << setw(3) << atk << "\n";
    }

    int attack(RandomEngine &r) override
    {
        int d = r.get(atk * 70 / 100, atk * 130 / 100);
        if (r.get(1, 100) <= 15)
        {
            d *= 2;
            cout << "REDEYE CRITICAL!\n";
        }
        return d;
    }

    void takeDamage(int d) override { hp = max(0, hp - d); }

    void save(ofstream &f) const override
    {
        f << en << ' ' << hp << ' ' << maxHp << ' ' << atk << '\n';
    }

    void load(ifstream &f) override
    {
        f >> en >> hp >> maxHp >> atk;
    }
};

// ==== BOSS 2: DarkKnight (บอสตัวใหม่) ====
class DarkKnight : public ICombat
{
    int hp, maxHp, atk, armor;

public:
    DarkKnight(int lv = 5) : hp(350 + lv * 20), maxHp(350 + lv * 20), atk(40 + lv * 6), armor(10) {}
    bool alive() const override { return hp > 0; }
    int getTypeID() const override { return 3; }

    void showStatus(string tag = "BOSS") const override
    {
        cout << left << setw(9) << tag << setw(14) << "Dark Knight"
             << "HP " << setw(3) << hp << "/" << setw(4) << maxHp
             << " ATK" << setw(3) << atk
             << " ARMOR" << setw(3) << armor << "\n";
    }

    int attack(RandomEngine &r) override
    {
        int d = r.get(atk * 80 / 100, atk * 120 / 100);
        if (r.get(1, 100) <= 20)
        {
            d += 15;
            cout << "DARK KNIGHT USES DARK SLASH!\n";
        }
        return d;
    }

    void takeDamage(int d) override 
    { 
        int reducedDamage = max(1, d - armor);
        hp = max(0, hp - reducedDamage); 
    }

    void save(ofstream &f) const override
    {
        f << hp << ' ' << maxHp << ' ' << atk << ' ' << armor << '\n';
    }

    void load(ifstream &f) override
    {
        f >> hp >> maxHp >> atk >> armor;
    }
};

// Game Engine
class GameEngine
{
    RandomEngine rng;

    bool run()
    {
        cout << "Trying to escape...\n";
        bool success = rng.get(1, 100) <= 60;
        cout << (success ? "Escaped!\n" : "Could not escape!\n");
        return success;
    }

    void enemyTurn(Hero<int> &player, ICombat &enemy, bool protect)
    {
        cout << "\n[ENEMY TURN]\n";
        if (protect)
        {
            cout << "[YOU] Protected! Damage reduced.\n";
            player.takeDamage(enemy.attack(rng) / 2);
        }
        else
        {
            int d = enemy.attack(rng);
            player.takeDamage(d);
            cout << "Enemy attacks! You received " << d << " damage.\n";
        }
    }

public:
    void battle(Hero<int> &player, ICombat &enemy, bool isBoss)
    {
        for (int turn = 1; player.alive() && enemy.alive(); turn++)
        {
            cout << "\n=== TURN " << turn << " ===\n";
            player.showStatus("[YOU]");
            enemy.showStatus(isBoss ? "[BOSS]" : "[ENEMY]");
            
            cout << "\n[1] Fight  [2] Charge  [3] Protect  [0] Save & Exit\n> ";
            int choice;
            cin >> choice;

            if (choice == 0) quitGame(&player, &enemy);

            bool protect = false;

            if (choice == 1)
            {
                cout << "[1] Attack  [2] Skill  [0] Back\n> ";
                int action;
                cin >> action;
                if (action == 0) { turn--; continue; }
                if (action == 1)
                {
                    int d = player.attack(rng);
                    enemy.takeDamage(d);
                    cout << "[YOU] Attack -> -" << d << " HP\n";
                }
                else if (action == 2)
                {
                    if (!player.skill(rng, enemy))
                    {
                        turn--;
                        continue;
                    }
                }
            }
            else if (choice == 2)
            {
                player.charge(rng);
            }
            else if (choice == 3)
            {
                protect = true;
                cout << "[YOU] Protected!\n";
            }
            else
            {
                turn--;
                continue;
            }

            if (enemy.alive())
            {
                enemyTurn(player, enemy, protect);
            }
        }

        reward(player, enemy, isBoss);
    }

    void reward(Hero<int> &player, ICombat &enemy, bool boss)
    {
        cout << "\n=== Result ===\n";
        if (!player.alive())
        {
            cout << "LOSE! You were defeated...\n";
            return;
        }

        int money = boss ? rng.get(100, 200) : rng.get(30, 70);
        int xp = boss ? rng.get(100, 150) : rng.get(30, 50);
        cout << (boss ? "BOSS DEFEATED!\n" : "VICTORY!\n")
             << "Money +" << money << " | XP +" << xp << "\n";
        player.addMoney(money);
        player.addXP(xp);
    }

    // ฟังก์ชัน encounter สุ่มมอนสเตอร์ธรรมดา หรือ สุ่ม Boss 2 ตัว
    void encounter(Hero<int> &player, const vector<Hero<int>> &list)
    {
        if (player.getLevel() < 3 || rng.get(1, 100) > 30)
        {
            Hero<int> enemy = list[rng.get(0, list.size() - 1)];
            cout << "\nA wild " << enemy.getName() << " appeared!\n";
            battle(player, enemy, false);
            return;
        }

        // สุ่ม Boss (1: Redeye, 2: DarkKnight)
        unique_ptr<ICombat> boss;
        int bossType = rng.get(1, 2);
        if (bossType == 1) boss = make_unique<Redeye>(player.getLevel());
        else boss = make_unique<DarkKnight>(player.getLevel());

        cout << "\n== BOSS APPEARED ==" << endl;
        boss->showStatus("[WARNING]");
        cout << "\n[1] FIGHT  [2] RUN  [3] Save & Exit\n> ";
        int choice;
        cin >> choice;

        if (choice == 3) quitGame(&player, boss.get());
        if (choice == 2)
        {
            if (run()) return;
            cout << "Boss attacks while escaping!\n";
            player.takeDamage(boss->attack(rng));
            if (!player.alive()) return;
        }

        battle(player, *boss, true);
    }
};

bool loadGame(Hero<int> &player, unique_ptr<ICombat> &savedEnemy, const vector<Hero<int>> &list)
{
    ifstream f("save.txt");
    if (!f) return false;
    
    player.load(f);
    int enemyType = 0;
    if (f >> enemyType && enemyType != 0)
    {
        if (enemyType == 1)
        {
            auto enemy = make_unique<Hero<int>>();
            enemy->load(f);
            savedEnemy = move(enemy);
        }
        else if (enemyType == 2)
        {
            auto boss = make_unique<Redeye>();
            boss->load(f);
            savedEnemy = move(boss);
        }
        else if (enemyType == 3)
        {
            auto boss = make_unique<DarkKnight>();
            boss->load(f);
            savedEnemy = move(boss);
        }
    }
    return true;
}

int main() 
{
    vector<Hero<int>> list = { {"Pikachu", 100, 30}, {"Charmander", 120, 25}, {"Squirtle", 140, 20} };
    RandomEngine rng;
    Hero<int> player;
    unique_ptr<ICombat> savedEnemy = nullptr;

    cout << "========== MINI RPG ==========\n[1] New Game\n[2] Load Save\n[0] Exit\n> ";
    int mode; 
    cin >> mode;

    if (mode == 0) quitGame();

    if (mode != 2 || !loadGame(player, savedEnemy, list)) {
        cout << "\n========== CHOOSE YOUR HERO ==========\n";
        for (int i = 0; i < (int)list.size(); ++i)
            cout << "[" << i << "] " << list[i].getName() << '\n';
        cout << "[99] Exit Game\n> ";
        int c; 
        cin >> c;
        if (c == 99 || c < 0 || c >= (int)list.size()) quitGame();
        player = list[c];
        cout << "\nNew Game Started with: " << player.getName() << '\n';
    } else {
        cout << "\nSave loaded successfully!\n";
    }

    GameEngine game;

    while (player.alive()) {
        if (savedEnemy) {
            cout << "\nContinuing fight with saved enemy...\n";
            bool isBoss = (savedEnemy->getTypeID() == 2 || savedEnemy->getTypeID() == 3);
            game.battle(player, *savedEnemy, isBoss);
            savedEnemy.reset();
        } else {
            cout << "\n========== CAMP ==========\n";
            player.showStatus("[YOU]");
            cout << "\n[1] Battle  [2] Heal ($30)  [0] Save & Exit\n> ";
            int choice; 
            cin >> choice;

            if (choice == 1) {
                game.encounter(player, list);
            } else if (choice == 2) {
                player.heal(rng);
            } else {
                quitGame(&player, nullptr);
            }
        }
    }

    cout << "\nGame Over! Thanks for playing.\n";
    return 0;
}