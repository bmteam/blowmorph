#ifndef BLOWMORPH_BASE_SINGLETON_H_
#define BLOWMORPH_BASE_SINGLETON_H_

// WARNING: Singleton is NOT thread safe.

template <typename T>
class Singleton {
 public:
  static T* GetInstance() {
    static T _instance;
    return &_instance;
  }

 private:
  // Forbid creating Singleton itself.
  Singleton();
  ~Singleton();
};

#endif // BLOWMORPH_BASE_SINGLETON_H_