/* Weapon.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Weapon.h"

#include "Audio.h"
#include "DataNode.h"
#include "Effect.h"
#include "GameData.h"
#include "Outfit.h"
#include "Logger.h"
#include "SpriteSet.h"

#include <algorithm>
#include <float.h>

using namespace std;

struct LoadWeaponFlags {
    bool isClustered;
    bool safeRangeOverriden;
    bool disabledDamageSet;
    bool minableDamageSet;
    bool relativeDisabledDamageSet;
    bool relativeMinableDamageSet;
};

template<class T>
T getAttArgOrDefault(json::reference v, int idx, T defVal) {
    T n = defVal;
    if (v.is_object()) {
        json arr = v.find("args").value();
        if (arr.size() > idx) {
            n = arr[idx];
        }
    } else if (v.is_array()) {
        if (v.size() > idx) {
            n = v[idx];
        }
    } else if (idx == 0) {
        n = v;
    }

    return n;
}

void handleEffect(std::map<const Effect *, int> &effects, json::reference v) {
    if (!v.is_array()) {
        Logger::LogError("Invalid effect definition");
        return;
    } else {
        for (json::iterator it = v.begin(); it != v.end(); ++it) {
            int count = 1;
            std::string effectName;

            json item = it.value();

            if (item.is_string()) {
                effectName = std::string(item);
            } else if (item.is_object()) {
                effectName = item.find("name").value();
                count = item.find("value").value();
            }

            effects[GameData::Effects().Get(effectName)] += count;
        }
    }
}

void Weapon::DBLoadWeapon(DBLoadWeaponArgs &args) {
    LoadWeaponFlags flags{};
    memset(&flags, 0, sizeof(LoadWeaponFlags));

    isWeapon = true;
    calculatedDamage = false;
    doesDamage = false;

    if (args.lifetime != nullptr) {
        lifetime = *args.lifetime;
    }

    if (args.velocity != nullptr) {
        velocity = *args.velocity;
    }

    if (args.reload != nullptr) {
        reload = *args.reload;
    }

    if (args.firingEnergy != nullptr) {
        firingEnergy = *args.firingEnergy;
    }

    if (args.firingHeat != nullptr) {
        firingHeat = *args.firingHeat;
    }

    if (args.inaccuracy != nullptr) {
        inaccuracy = *args.inaccuracy;
    }

    if (args.shieldDamage != nullptr) {
        damage[SHIELD_DAMAGE] = *args.shieldDamage;
    }

    if (args.hullDamage != nullptr) {
        damage[HULL_DAMAGE] = *args.hullDamage;
    }

    if (args.weaponAttributes != nullptr) {
        json j = json::parse(*args.weaponAttributes);

        for (json::iterator it = j.begin(); it != j.end(); ++it) {
            string key = it.key();

            if(key == "sprite")
                sprite.JsonLoadSprite(it.value());
            else if(key == "hardpoint sprite")
                hardpointSprite.JsonLoadSprite(it.value());
            else if(key == "sound")
                sound = Audio::Get(it.value());
            else if(key == "icon")
                icon = SpriteSet::Get(it.value());
            else if(key == "ammo") {
                std::string s = getAttArgOrDefault<std::string>(it.value(), 0, "");
                int count = getAttArgOrDefault<int>(it.value(), 1, 1);
                ammo = make_pair(GameData::Outfits().Get(s), count);
            }
            else if(key == "fire effect")
            {
                handleEffect(fireEffects, it.value());
            }
            else if(key == "live effect")
            {
                handleEffect(liveEffects, it.value());
            }
            else if(key == "hit effect")
            {
                handleEffect(hitEffects, it.value());
            }
            else if(key == "target effect")
            {
                handleEffect(targetEffects, it.value());
            }
            else if(key == "die effect")
            {
                handleEffect(dieEffects, it.value());
            }
            else if(key == "hardpoint offset") {
                double first = getAttArgOrDefault<double>(it.value(), 0, 0.0);
                double second = getAttArgOrDefault<double>(it.value(), 1, DBL_MAX);

                // A single value specifies the y-offset, while two values
                // specifies an x & y offset, e.g. for an asymmetric hardpoint.
                // The point is specified in traditional XY orientation, but must
                // be inverted along the y-dimension for internal use.
                if(second == DBL_MAX)
                    hardpointOffset = Point(0., -first);
                else
                    hardpointOffset = Point(first, -second);
            } else if(key == "damage dropoff") {
                hasDamageDropoff = true;
                double dropoff = getAttArgOrDefault<double>(it.value(), 0, 0.0);
                double maxDropoff = getAttArgOrDefault<double>(it.value(), 1, 0.0);
                damageDropoffRange = make_pair(max(0., dropoff), maxDropoff);
            }
            else if(key == "submunition") {
                if (it.value().is_array()) {
                    for (auto & grand : it.value()) {
                        std::string name = grand.find("name").value();

                        int count = 1;
                        if (grand.contains("count")) {
                            count = grand.find("count").value();
                        }

                        submunitions.emplace_back(GameData::Outfits().Get(name), count);
                        if (grand.contains("facing")) {
                            double facing = grand.find("facing").value();
                            json offset = grand.find("offset").value();
                            double x = getAttArgOrDefault<double>(offset, 0, 0.0);
                            double y = getAttArgOrDefault<double>(offset, 1, 0.0);

                            submunitions.back().facing = Angle(facing);
                            submunitions.back().offset = Point(x, y);
                        }
                    }
                }
            }
            else if (it.value().is_boolean()) {
                handleBoolParam(key, flags);
            } else if (it.value().is_number()) {
                handleDoubleParam(key, it.value(), flags);
            } else {
                Logger::LogError("Unrecognized weapon attribute: \"" + key + "\":");
            }
        }
    }

    if (args.spriteArgs != nullptr) {
        sprite.DBLoadSprite(*args.spriteArgs);
    }

    handleFlags(flags);
    sanityChecks();
}


// Load from a "weapon" node, either in an outfit or in a ship (explosion).
void Weapon::LoadWeapon(const DataNode &node)
{
    LoadWeaponFlags flags{};
    memset(&flags, 0, sizeof(LoadWeaponFlags));

    isWeapon = true;
    calculatedDamage = false;
    doesDamage = false;

    for(const DataNode &child : node)
    {
        const string &key = child.Token(0);
        if (child.Size() == 1)
            handleBoolParam(key, flags);
        else if(key == "sprite")
            sprite.LoadSprite(child);
        else if(key == "hardpoint sprite")
            hardpointSprite.LoadSprite(child);
        else if(key == "sound")
            sound = Audio::Get(child.Token(1));
        else if(key == "ammo")
        {
            int usage = (child.Size() >= 3) ? child.Value(2) : 1;
            ammo = make_pair(GameData::Outfits().Get(child.Token(1)), max(0, usage));
        }
        else if(key == "icon")
            icon = SpriteSet::Get(child.Token(1));
        else if(key == "fire effect")
        {
            int count = (child.Size() >= 3) ? child.Value(2) : 1;
            fireEffects[GameData::Effects().Get(child.Token(1))] += count;
        }
        else if(key == "live effect")
        {
            int count = (child.Size() >= 3) ? child.Value(2) : 1;
            liveEffects[GameData::Effects().Get(child.Token(1))] += count;
        }
        else if(key == "hit effect")
        {
            int count = (child.Size() >= 3) ? child.Value(2) : 1;
            hitEffects[GameData::Effects().Get(child.Token(1))] += count;
        }
        else if(key == "target effect")
        {
            int count = (child.Size() >= 3) ? child.Value(2) : 1;
            targetEffects[GameData::Effects().Get(child.Token(1))] += count;
        }
        else if(key == "die effect")
        {
            int count = (child.Size() >= 3) ? child.Value(2) : 1;
            dieEffects[GameData::Effects().Get(child.Token(1))] += count;
        }
        else if(key == "submunition")
        {
            submunitions.emplace_back(
                    GameData::Outfits().Get(child.Token(1)),
                    (child.Size() >= 3) ? child.Value(2) : 1);
            for(const DataNode &grand : child)
            {
                if((grand.Size() >= 2) && (grand.Token(0) == "facing"))
                    submunitions.back().facing = Angle(grand.Value(1));
                else if((grand.Size() >= 3) && (grand.Token(0) == "offset"))
                    submunitions.back().offset = Point(grand.Value(1), grand.Value(2));
                else
                    child.PrintTrace("Skipping unknown or incomplete submunition attribute:");
            }
        } else if(key == "hardpoint offset") {
            double value = child.Value(1);
            // A single value specifies the y-offset, while two values
            // specifies an x & y offset, e.g. for an asymmetric hardpoint.
            // The point is specified in traditional XY orientation, but must
            // be inverted along the y-dimension for internal use.
            if(child.Size() == 2)
                hardpointOffset = Point(0., -value);
            else if(child.Size() == 3)
                hardpointOffset = Point(value, -child.Value(2));
            else
                child.PrintTrace("Unsupported \"" + key + "\" specification:");
        } else if(key == "damage dropoff") {
            hasDamageDropoff = true;
            double maxDropoff = (child.Size() >= 3) ? child.Value(2) : 0.;
            damageDropoffRange = make_pair(max(0., child.Value(1)), maxDropoff);
        } else if(key == "inaccuracy") {
            inaccuracy = child.Value(1);
            for(const DataNode &grand : child)
            {
                for(int j = 0; j < grand.Size(); ++j)
                {
                    const string &token = grand.Token(j);

                    if(token == "inverted")
                        inaccuracyDistribution.second = true;
                    else if(token == "triangular")
                        inaccuracyDistribution.first = Distribution::Type::Triangular;
                    else if(token == "uniform")
                        inaccuracyDistribution.first = Distribution::Type::Uniform;
                    else if(token == "narrow")
                        inaccuracyDistribution.first = Distribution::Type::Narrow;
                    else if(token == "medium")
                        inaccuracyDistribution.first = Distribution::Type::Medium;
                    else if(token == "wide")
                        inaccuracyDistribution.first = Distribution::Type::Wide;
                    else
                        grand.PrintTrace("Skipping unknown distribution attribute:");
                }
            }
        } else {
            handleDoubleParam(key, child.Value(1), flags);
        }
    }

    sanityChecks();
}

void Weapon::handleBoolParam(const string &key, LoadWeaponFlags &flags) {
    if(key == "stream")
        isStreamed = true;
    else if(key == "safe")
        isSafe = true;
    else if(key == "phasing")
        isPhasing = true;
    else if(key == "no damage scaling")
        isDamageScaled = false;
    else if(key == "parallel")
        isParallel = true;
    else if(key == "gravitational")
        isGravitational = true;
    else if(key == "cluster")
        flags.isClustered = true;
    else
        Logger::LogError("Unrecognized weapon attribute: \"" + key + "\":");
}

void Weapon::handleFlags(LoadWeaponFlags &flags) {
    // Disabled damage defaults to hull damage instead of 0.
    if(!flags.disabledDamageSet)
        damage[DISABLED_DAMAGE] = damage[HULL_DAMAGE];
    if(!flags.relativeDisabledDamageSet)
        damage[RELATIVE_DISABLED_DAMAGE] = damage[RELATIVE_HULL_DAMAGE];
    // Minable damage defaults to hull damage instead of 0.
    if(!flags.minableDamageSet)
        damage[MINABLE_DAMAGE] = damage[HULL_DAMAGE];
    if(!flags.relativeMinableDamageSet)
        damage[RELATIVE_MINABLE_DAMAGE] = damage[RELATIVE_HULL_DAMAGE];

    // Weapons of the same type will alternate firing (streaming) rather than firing all
    // at once (clustering) if the weapon is not a special weapon type (e.g. anti-missile,
    // tractor beam) and is not vulnerable to anti-missile, or has the "stream" attribute.
    isStreamed |= !(MissileStrength() || AntiMissile() || TractorBeam());
    isStreamed &= !flags.isClustered;

    // Only when the weapon is not safe and has a blast radius is safeRange needed,
    // except if it is already overridden.
    if(!isSafe && blastRadius > 0 && !flags.safeRangeOverriden)
        safeRange = (blastRadius + triggerRadius);
}

void Weapon::handleDoubleParam(const string &key, double value, LoadWeaponFlags &flags) {
    if(key == "lifetime")
        lifetime = max(0., value);
    else if(key == "random lifetime")
        randomLifetime = max(0., value);
    else if(key == "fade out")
        fadeOut = max(0., value);
    else if(key == "reload")
        reload = max(1., value);
    else if(key == "burst reload")
        burstReload = max(1., value);
    else if(key == "burst count")
        burstCount = max(1., value);
    else if(key == "homing")
        homing = value;
    else if(key == "missile strength")
        missileStrength = max(0., value);
    else if(key == "anti-missile")
        antiMissile = max(0., value);
    else if(key == "tractor beam")
        tractorBeam = max(0., value);
    else if(key == "penetration count")
        penetrationCount = static_cast<uint16_t>(value);
    else if(key == "velocity")
        velocity = value;
    else if(key == "random velocity")
        randomVelocity = value;
    else if(key == "acceleration")
        acceleration = value;
    else if(key == "drag")
        drag = value;
    else if(key == "turn")
        turn = value;
    else if(key == "turret turn")
        turretTurn = value;
    else if(key == "tracking")
        tracking = max(0., min(1., value));
    else if(key == "optical tracking")
        opticalTracking = max(0., min(1., value));
    else if(key == "infrared tracking")
        infraredTracking = max(0., min(1., value));
    else if(key == "radar tracking")
        radarTracking = max(0., min(1., value));
    else if(key == "firing energy")
        firingEnergy = value;
    else if(key == "firing force")
        firingForce = value;
    else if(key == "firing fuel")
        firingFuel = value;
    else if(key == "firing heat")
        firingHeat = value;
    else if(key == "firing hull")
        firingHull = value;
    else if(key == "firing shields")
        firingShields = value;
    else if(key == "firing ion")
        firingIon = value;
    else if(key == "firing scramble")
        firingScramble = value;
    else if(key == "firing slowing")
        firingSlowing = value;
    else if(key == "firing disruption")
        firingDisruption = value;
    else if(key == "firing discharge")
        firingDischarge = value;
    else if(key == "firing corrosion")
        firingCorrosion = value;
    else if(key == "firing leak")
        firingLeak = value;
    else if(key == "firing burn")
        firingBurn = value;
    else if(key == "relative firing energy")
        relativeFiringEnergy = value;
    else if(key == "relative firing heat")
        relativeFiringHeat = value;
    else if(key == "relative firing fuel")
        relativeFiringFuel = value;
    else if(key == "relative firing hull")
        relativeFiringHull = value;
    else if(key == "relative firing shields")
        relativeFiringShields = value;
    else if(key == "split range")
        splitRange = max(0., value);
    else if(key == "trigger radius")
        triggerRadius = max(0., value);
    else if(key == "blast radius")
        blastRadius = max(0., value);
    else if(key == "shield damage")
        damage[SHIELD_DAMAGE] = value;
    else if(key == "hull damage")
        damage[HULL_DAMAGE] = value;
    else  if(key == "fuel damage")
        damage[FUEL_DAMAGE] = value;
    else if(key == "heat damage")
        damage[HEAT_DAMAGE] = value;
    else if(key == "energy damage")
        damage[ENERGY_DAMAGE] = value;
    else if(key == "ion damage")
        damage[ION_DAMAGE] = value;
    else if(key == "scrambling damage")
        damage[WEAPON_JAMMING_DAMAGE] = value;
    else if(key == "disruption damage")
        damage[DISRUPTION_DAMAGE] = value;
    else if(key == "slowing damage")
        damage[SLOWING_DAMAGE] = value;
    else if(key == "discharge damage")
        damage[DISCHARGE_DAMAGE] = value;
    else if(key == "corrosion damage")
        damage[CORROSION_DAMAGE] = value;
    else if(key == "leak damage")
        damage[LEAK_DAMAGE] = value;
    else if(key == "burn damage")
        damage[BURN_DAMAGE] = value;
    else if(key == "relative shield damage")
        damage[RELATIVE_SHIELD_DAMAGE] = value;
    else if(key == "relative hull damage")
        damage[RELATIVE_HULL_DAMAGE] = value;
     else if(key == "relative fuel damage")
        damage[RELATIVE_FUEL_DAMAGE] = value;
    else if(key == "relative heat damage")
        damage[RELATIVE_HEAT_DAMAGE] = value;
    else if(key == "relative energy damage")
        damage[RELATIVE_ENERGY_DAMAGE] = value;
    else if(key == "hit force")
        damage[HIT_FORCE] = value;
    else if(key == "piercing")
        piercing = max(0., value);
    else if(key == "prospecting")
        prospecting = value;
    else if(key == "range override")
        rangeOverride = max(0., value);
    else if(key == "velocity override")
        velocityOverride = max(0., value);
    else if(key == "dropoff modifier")
        damageDropoffModifier = max(0., value);
    else if(key == "safe range override") {
        safeRange = max(0., value);
        flags.safeRangeOverriden = true;
    } else if(key == "disabled damage") {
        damage[DISABLED_DAMAGE] = value;
        flags.disabledDamageSet = true;
    } else if(key == "minable damage") {
        damage[MINABLE_DAMAGE] = value;
        flags.minableDamageSet = true;
    } else if(key == "relative disabled damage") {
        damage[RELATIVE_DISABLED_DAMAGE] = value;
        flags.relativeDisabledDamageSet = true;
    } else if(key == "relative minable damage") {
        damage[RELATIVE_MINABLE_DAMAGE] = value;
        flags.relativeMinableDamageSet = true;
    }
    else
        Logger::LogError("Unrecognized weapon attribute: \"" + key + "\":");
}


void Weapon::sanityChecks() {
	// Sanity checks:
	if(burstReload > reload)
		burstReload = reload;
	if(damageDropoffRange.first > damageDropoffRange.second)
		damageDropoffRange.second = Range();

	// Support legacy missiles with no tracking type defined:
	if(homing && !tracking && !opticalTracking && !infraredTracking && !radarTracking)
	{
		tracking = 1.;
        Logger::LogError("Warning: Deprecated use of \"homing\" without use of \"[optical|infrared|radar] tracking.\"");
	}

	// Convert the "live effect" counts from occurrences per projectile lifetime
	// into chance of occurring per frame.
	if(lifetime <= 0)
		liveEffects.clear();
	for(auto it = liveEffects.begin(); it != liveEffects.end(); )
	{
		if(!it->second)
			it = liveEffects.erase(it);
		else
		{
			it->second = max(1, lifetime / it->second);
			++it;
		}
	}

}



bool Weapon::IsWeapon() const
{
	return isWeapon;
}



// Get assets used by this weapon.
const Body &Weapon::WeaponSprite() const
{
	return sprite;
}



const Body &Weapon::HardpointSprite() const
{
	return hardpointSprite;
}



const Sound *Weapon::WeaponSound() const
{
	return sound;
}



const Outfit *Weapon::Ammo() const
{
	return ammo.first;
}



int Weapon::AmmoUsage() const
{
	return ammo.second;
}



bool Weapon::IsParallel() const
{
	return isParallel;
}



const Sprite *Weapon::Icon() const
{
	return icon;
}



// Effects to be created at the start or end of the weapon's lifetime.
const map<const Effect *, int> &Weapon::FireEffects() const
{
	return fireEffects;
}



const map<const Effect *, int> &Weapon::LiveEffects() const
{
	return liveEffects;
}



const map<const Effect *, int> &Weapon::HitEffects() const
{
	return hitEffects;
}



const map<const Effect *, int> &Weapon::TargetEffects() const
{
	return targetEffects;
}



const map<const Effect *, int> &Weapon::DieEffects() const
{
	return dieEffects;
}



const vector<Weapon::Submunition> &Weapon::Submunitions() const
{
	return submunitions;
}



double Weapon::TotalLifetime() const
{
	if(rangeOverride)
		return rangeOverride / WeightedVelocity();
	if(totalLifetime < 0.)
	{
		totalLifetime = 0.;
		for(const auto &it : submunitions)
			totalLifetime = max(totalLifetime, it.weapon->TotalLifetime());
		totalLifetime += lifetime;
	}
	return totalLifetime;
}



double Weapon::Range() const
{
	return (rangeOverride > 0) ? rangeOverride : WeightedVelocity() * TotalLifetime();
}



// Calculate the fraction of full damage that this weapon deals given the
// distance that the projectile traveled if it has a damage dropoff range.
double Weapon::DamageDropoff(double distance) const
{
	double minDropoff = damageDropoffRange.first;
	double maxDropoff = damageDropoffRange.second;

	if(distance <= minDropoff)
		return 1.;
	if(distance >= maxDropoff)
		return damageDropoffModifier;
	// Damage modification is linear between the min and max dropoff points.
	double slope = (1 - damageDropoffModifier) / (minDropoff - maxDropoff);
	return slope * (distance - minDropoff) + 1;
}



// Return the weapon's damage dropoff at maximum range.
double Weapon::MaxDropoff() const
{
	return damageDropoffModifier;
}



// Return the ranges at which the weapon's damage dropoff begins and ends.
const pair<double, double> &Weapon::DropoffRanges() const
{
	return damageDropoffRange;
}



// Legacy support: allow turret outfits with no turn rate to specify a
// default turnrate.
void Weapon::SetTurretTurn(double rate)
{
	turretTurn = rate;
}



double Weapon::TotalDamage(int index) const
{
	if(!calculatedDamage)
	{
		calculatedDamage = true;
		for(int i = 0; i < DAMAGE_TYPES; ++i)
		{
			for(const auto &it : submunitions)
				damage[i] += it.weapon->TotalDamage(i) * it.count;
			doesDamage |= (damage[i] > 0.);
		}
	}
	return damage[index];
}



pair<Distribution::Type, bool> Weapon::InaccuracyDistribution() const
{
	return inaccuracyDistribution;
}



double Weapon::Inaccuracy() const
{
	return inaccuracy;
}
