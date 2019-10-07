TODO
====

## gmath library
  * bigint

## Implmentation details
> None


## Event handling
Publisher - Subscriber

- Publisher knows about the subscribers (std::map, pointers to subscribers)
- Publisher pass itself to subscribers via an update method

class Publisher {
public:
    virtual void attach(Subscriber* sub);       // Add sub
    virtual void detach(Subscriber* sub);       // Remove sub
    virtual void notify();                      // foreach subs update(this)

protected:
    Publisher();

private:
    std::vector<Subscriber*> _subs;
}

class Subscriber {
public:
    virtual ~Subscriber();                      // in subclass: { _sub->detach(this); }
    virtual void update(Publisher* obj) = 0;

protected:
    Subscriber();
}