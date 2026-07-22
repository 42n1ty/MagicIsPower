namespace game {
  
  enum class ItemRarity : uint32_t {
    Junk = 1 << 0,      //grey
    Common = 1 << 1,    //white
    Uncommon = 1 << 2,  //green
    Rare = 1 << 3,     //blue
    Epic = 1 << 4,      //purple
    Legendary = 1 << 5, //orange (craft blocked)
    Mythic = 1 << 6,    //red (endgame base)
    Artifact = 1 << 7,  //cyan (open slot(s))
    Divine = 1 << 8,    //magenta
    Relic = 1 << 9      //gold (craft blocked, soulbound)
  };
  
  enum class ModTier : uint16_t {
    T0 = 1 << 0,
    T1 = 1 << 1,
    T2 = 1 << 2,
    T3 = 1 << 3,
    T4 = 1 << 4,
    T5 = 1 << 5,
    T6 = 1 << 6,
    T7 = 1 << 7,
    T8 = 1 << 8,
    T9 = 1 << 9,
    T10 = 1 << 10
  };
  
  enum class ItemCategory : uint32_t {
    Weapon = 1 << 0,
    Armor = 1 << 1,
    Jewelry = 1 << 2,
    SkillGem = 1 << 3
  };
  
  enum class FilterAction {
    Show,
    Hide,
    AutoSalvage
  };
  
  struct LootFilterRool {
    bool enable = true;
    std::string name;
    
    uint32_t rarityMask = 0xFFFFFFFF;
    uint32_t categoryMask = 0xFFFFFFFF;
    
    FilterAction action = FilterAction::Show;
    
  };
  
};