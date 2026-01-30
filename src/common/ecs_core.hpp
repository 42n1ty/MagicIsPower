#pragma once

#include <vector>
#include <string>
#include <memory>
#include <typeindex>
#include <functional>
#include <expected>
#include <optional>
#include <concepts>
#include <limits>
#include <algorithm>
#include <atomic>
#include <bitset>
#include <cassert>
#include <unordered_map>


namespace ecs {
  
  using EntID = uint32_t;
  using TypeID = uint32_t;
  constexpr EntID NULL_ENT = std::numeric_limits<EntID>::max();
  constexpr size_t MAX_COMPONENTS = 64;
  using Signature = std::bitset<MAX_COMPONENTS>;
  
  inline TypeID getNextID() {
    static std::atomic<TypeID> counter{0};
    return counter++;
  }
  
  template <typename T>
  inline TypeID getComponentID() {
    static const TypeID id = getNextID();
    return id;
  }
  
  struct StrTransparentHash {
    using is_transparent = void;
    size_t operator()(const char * txt) const {return std::hash<std::string_view>{}(txt);}
    size_t operator()(std::string_view txt) const {return std::hash<std::string_view>{}(txt);}
    size_t operator()(const std::string& txt) const {return std::hash<std::string>{}(txt);}
  };
  
  template <typename T>
  struct Handle {
    uint32_t index = std::numeric_limits<uint32_t>::max();
    uint32_t gen = 0;
    
    auto operator<=>(const Handle&) const = default;
    
    [[nodiscard]] bool isValid() const {
      return index != std::numeric_limits<uint32_t>::max();
    }
  };
  
  struct rawHandle { uint32_t index; uint32_t gen; };
  
  
  class Manager;

  struct ISystem {
    virtual ~ISystem() = default;
    virtual void update(Manager& ecs, float dt) = 0;
  };
  
  
  //==========================================
  // ASSETS
  //==========================================
  
  struct IAssetStor {
    virtual ~IAssetStor() = default;
    virtual void clear() = 0;
  };
  
  //loader concept
  template <typename L, typename T>
  concept AssetLoaderConc = requires(L& l, const std::string& p) {
    {l.load(p)} -> std::same_as<std::expected<T, std::string_view>>;
  };
  
  template <typename T>
  class AssetPool : public IAssetStor {
  public:
    struct Slot {
      T asset;
      uint32_t gen = 0;
      bool active = false;
    };
    
  private:
    std::vector<Slot> slots;
    std::vector<uint32_t> freeInds;
    std::unordered_map<std::string, uint32_t, StrTransparentHash, std::equal_to<>> lookup;
    
  public:
    AssetPool() = default;
    AssetPool(const AssetPool&) = delete;
    AssetPool& operator=(const AssetPool&) = delete;
    
    void clear() override {
      slots.clear();
      freeInds.clear();
      lookup.clear();
    }
    
    rawHandle insert(std::string_view key, T&& asset) {
      uint32_t idx;
      if(!freeInds.empty()) {
        idx = freeInds.back();
        freeInds.pop_back();
      }
      else {
        idx = static_cast<uint32_t>(slots.size());
        slots.emplace_back();
      }
      
      Slot& slot = slots[idx];
      slot.asset = std::move(asset);
      slot.active = true;
      
      if(!key.empty()) {
        lookup[std::string(key)] = idx;
      }
      
      return {idx, slot.gen};
    }
    
    T* get(Handle<T> h) {
      if(h.index >= slots.size()) return nullptr;
      Slot& slot = slots[h.index];
      if(!slot.active || slot.gen != h.gen) return nullptr;
      
      return &slot.asset;
    }
    
    void unload(Handle<T> h) {
      if(Get(h)) {
        Slot& slot = slots[h.index];
        slot.active = false;
        slot.gen++;
        slot.asset = T();
        freeInds.emplace_back(h.index);
      }
    }
    
    std::optional<rawHandle> find(std::string_view key) {
      if(auto it = lookup.find(key); it != lookup.end()) {
        uint32_t idx = it->second;
        if(slots[idx].active) {
          return rawHandle{idx, slots[idx].gen};
        }
      }
      
      return std::nullopt;
    }
    
  };
  
  //==========================================
  // COMPONENTS
  //==========================================
  
  struct IComponentStor {
    virtual ~IComponentStor() = default;
    virtual void remove(EntID id) = 0;
  };
  
  template <typename T>
  class SparseSet : public IComponentStor {
    std::vector<T> dense;
    std::vector<EntID> sparse;
    std::vector<EntID> backlink;
    
  public:
    T& add(EntID e, T&& comp) {
      if(e >= sparse.size()) {
        sparse.resize(e + 13, NULL_ENT);
      }
      
      if(sparse[e] != NULL_ENT) {
        dense[sparse[e]] = std::move(comp);
        return dense[sparse[e]];
      }
      
      sparse[e] = static_cast<EntID>(dense.size());
      backlink.emplace_back(e);
      dense.emplace_back(std::move(comp));
      
      return dense.back();
    }
    
    void remove(EntID e) override {
      if(e >= sparse.size() || sparse[e] == NULL_ENT) return;
      
      EntID removeIdx = sparse[e];
      EntID lastIdx = static_cast<EntID>(dense.size() - 1);
      EntID lastE = backlink[lastIdx];
      if(removeIdx != lastIdx) {
        std::swap(dense[removeIdx], dense[lastIdx]);
        std::swap(backlink[removeIdx], backlink[lastIdx]);
        
        sparse[lastE] = removeIdx;
      }
      
      dense.pop_back();
      backlink.pop_back();
      sparse[e] = NULL_ENT;
    }
    
    T* get(EntID e) {
      if(e >= sparse.size() || sparse[e] == NULL_ENT) return nullptr;
      
      return &dense[sparse[e]];
    }
    
    bool has(EntID e) {
      return e < sparse.size() && sparse[e] != NULL_ENT;
    }
    
    auto begin() {return dense.begin();}
    auto end() {return dense.end();}
    
    const std::vector<EntID>& getOwners() const {return backlink;}
  };
  
  //==========================================
  // MANAGER
  //==========================================
  
  class Manager {
    std::vector<std::unique_ptr<IComponentStor>> storsOwnership;
    std::vector<IComponentStor*> storsID;
    std::vector<Signature> signatures;
    std::vector<std::unique_ptr<ISystem>> systems;
    
    std::unordered_map<std::type_index, std::unique_ptr<IAssetStor>> assets;
    std::unordered_map<std::type_index, std::function<rawHandle(const std::string&)>> assetLoaders;
    
    EntID nextE = 0;
    
  public:
    
    Manager() = default;
    
    EntID createEntity() {
      EntID id = nextE++;
      if(signatures.size() <= id) {
        signatures.resize(id + 13);
      }
      signatures[id].reset();
      
      return id;
    }
    
    void destroyEntity(EntID e) {
      Signature& mask = signatures[e];
      
      for(size_t i = 0; i < storsID.size(); ++i) {
        if(mask.test(i)) {
          storsID[i]->remove(e);
        }
      }
      mask.reset();
    }
    
    const Signature& getSignature(EntID e) {
      return signatures[e];
    }
    
    //==========================================
    // COMPONENT MANAGEMENT
    //==========================================
    
    template <typename T>
    void registerComponent() {
      TypeID id = getComponentID<T>();
      
      assert(id < MAX_COMPONENTS && "Too many component types registered!");
      
      if(storsID.size() <= id) {
        storsID.resize(id + 1, nullptr);
      }
      
      auto newStor = std::make_unique<SparseSet<T>>();
      storsID[id] = newStor.get();
      storsOwnership.emplace_back(std::move(newStor));
    }
    
    template <typename T>
    T& addComponent(EntID e, T&& comp) {
      TypeID id = getComponentID<T>();
      
      assert(id < storsID.size() && storsID[id] && "Component not registered before use!");
      
      signatures[e].set(id, true);
      
      return static_cast<SparseSet<T>*>(storsID[id])->add(e, std::move(comp));
    }
    
    template <typename T>
    void removeComponent(EntID e) {
      TypeID id = getComponentID<T>();
      signatures[e].set(id, false);
      storsID[id]->remove(e);
    }
    
    template <typename T>
    T* getComponent(EntID e) {
      TypeID id = getComponentID<T>();
      if(!signatures[e].test(id)) return nullptr;
      
      return static_cast<SparseSet<T>*>(storsID[id])->get(e);
    }
    
    template <typename T>
    SparseSet<T>& view() {
      auto typeId = getComponentID<T>();
      assert(typeId < storsID.size() && storsID[typeId]);
      return *static_cast<SparseSet<T>*>(storsID[typeId]);
    }
    
    //==========================================
    // ASSET MANAGEMENT
    //==========================================
    
    // loader API - std::expected<T, std::string> load(const std::string&)
    template <typename T, typename L>
    void registerAsset(L&& l) {
      auto pool = std::make_unique<AssetPool<T>>();
      AssetPool<T>* rawPool = pool.get();
      assets[std::type_index(typeid(T))] = std::move(pool);
      
      assetLoaders[std::type_index(typeid(T))] = [rawPool, loader = std::forward<L>(l)](const std::string& path) mutable -> rawHandle {
        if(auto existing = rawPool->find(path)) {
          return *existing;
        }
        
        auto result = loader.load(path);
        if(!result) {
          return {std::numeric_limits<uint32_t>::max(), 0};
        }
        
        return rawPool->insert(path, std::move(result.value()));
      };
    }
    
    template <typename T>
    Handle<T> loadAsset(const std::string& path) {
      auto it = assetLoaders.find(std::type_index(typeid(T)));
      assert(it != assetLoaders.end() && "Asset type not registered!");
      rawHandle raw = it->second(path);
      
      return Handle<T>{raw.index, raw.gen};
    }
    
    template <typename T>
    T* getAsset(Handle<T> h) {
      auto it = assets.find(std::type_index(typeid(T)));
      if(it == assets.end()) return nullptr;
      
      return static_cast<AssetPool<T>*>(it->second.get())->get(h);
    }
    
    //==========================================
    // ASSET MANAGEMENT
    //==========================================
    
    template <typename S, typename... Args>
    S& registerSystem(Args&&... args) {
      static_assert(std::is_base_of<ISystem, S>::value, "System must inherit from ISystem");
      
      auto sys = std::make_unique<S>(std::forward<Args>(args)...);
      S* ptr = sys.get();
      systems.emplace_back(std::move(sys));
      
      return *ptr;
    }
    
    void update(float dt) {
      for(auto& sys : systems) {
        sys->update(*this, dt);
      }
    }
  };
  
}; //ecs