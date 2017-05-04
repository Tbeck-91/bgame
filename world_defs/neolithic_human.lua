-- Under-developed humans
species_sentient['neolithic_human'] = {
    name = "Human", male_name = "Man", female_name = "Woman", group_name = "People",
    description = "A bipedal ape-descendant with an unhealthy obsession with money and power",
    stat_mods = {}, -- Humans have no modifiers
    parts = humanoid_parts,
    ethics = { diet="omnivore", alignment="neutral" },
    max_age = 114, infant_age=3, child_age=16,
    glyph = glyphs['at'], worldgen_glyph = glyphs['caveman'],
    composite_render=true, base_male=glyphs['base_male'], base_female=glyphs['base_female'],
    skin_colors = human_skin_tones, hair_colors = human_hair_colors
}

neolithic_human_leader = {
    n = 2, name = "Tribal Leader", level=2,
    armor_class = 10,
    natural_attacks = {
        fist = { type = "fists", hit_bonus = 0, n_dice = 1, die_type = 4, die_mod = 0 }
    },
    equipment = {
        both = { torso="tunic/hide", shoes="sandals/hide" },
        male = { legs="britches/hide" },
        female = { legs="skirt_simple/hide" },
        melee = "warhammer/granite"
    },
    hp_n = 1, hp_dice = 10, hp_mod = 1,
    gender = "random"
}
neolithic_human_shaman = {
    n = 4, name = "Tribal Shaman", level=2,
    armor_class = 10,
    natural_attacks = {
        fist = { type = "fists", hit_bonus = 0, n_dice = 1, die_type = 4, die_mod = 0 }
    },
    equipment = {
        both = { torso="tunic/hide", shoes="sandals/hide" },
        male = { legs="britches/hide" },
        female = { legs="skirt_simple/hide" },
        melee = "knife/granite"
    },
    hp_n = 1, hp_dice = 10, hp_mod = 1,
    gender = "random"
}
neolithic_human_worker = {
    n = 20, name = "Tribal Worker", level=1,
    armor_class = 10,
    natural_attacks = {
        fist = { type = "fists", hit_bonus = 0, n_dice = 1, die_type = 4, die_mod = 0 }
    },
    equipment = {
        both = { torso="tunic/hide", shoes="sandals/hide" },
        male = { legs="britches/hide" },
        female = { legs="skirt_simple/hide" },
        melee = "knife/granite"
    },
    hp_n = 1, hp_dice = 10, hp_mod = 1,
    gender = "random"
}
neolithic_human_guard = {
    n = 10, name = "Tribal Guard", level=1,
    armor_class = 10,
    natural_attacks = {
        fist = { type = "fists", hit_bonus = 0, n_dice = 1, die_type = 4, die_mod = 0 }
    },
    equipment = {
        both = { torso="tunic/leather", shoes="sandals/leather", head="cap/leather" },
        male = { legs="britches/leather" },
        female = { legs="skirt_simple/leather" },
        melee = "knife/granite"
    },
    hp_n = 1, hp_dice = 10, hp_mod = 1,
    gender = "random"
}
neolithic_human_hunter = {
    n = 5, name = "Tribal Hunter", level=1,
    armor_class = 10,
    natural_attacks = {
        fist = { type = "fists", hit_bonus = 0, n_dice = 1, die_type = 4, die_mod = 0 }
    },
    equipment = {
        both = { torso="tunic/leather", shoes="sandals/leather", head="cap/leather" },
        male = { legs="britches/leather" },
        female = { legs="skirt_simple/leather" },
        melee = "knife/granite", ranged = "atlatl/wood", ammo = "dart/wood"
    },
    hp_n = 1, hp_dice = 10, hp_mod = 1,
    gender = "random"
}
neolithic_human_raider = {
    n = 25, name = "Tribal Raider", level=1,
    armor_class = 10,
    natural_attacks = {
        fist = { type = "fists", hit_bonus = 0, n_dice = 1, die_type = 4, die_mod = 0 }
    },
    equipment = {
        both = { torso="tunic/boiled_leather", shoes="sandals/boiled_leather", head="cap/boiled_leather" },
        male = { legs="britches/boiled_leather" },
        female = { legs="skirt_simple/boiled_leather" },
        melee = "dagger/granite", ranged = "shortbow/wood", ammo = "arrow/wood"
    },
    hp_n = 1, hp_dice = 10, hp_mod = 1,
    gender = "random"
}

civilizations['neolithic_human'] = {
    tech_level = 0,
    species_def = 'neolithic_human',
    ai = 'builder',
    name_generator = "neohuman",
    can_build = { "earthworks", "wood-pallisade", "hut", "shrine", "well", "farrier", "butcher" },
    units = {
        garrison = {
            bp_per_turn = 5,
            speed = 0,
            name = "Neolothic Tribe",
            sentients = {
                leader = neolithic_human_leader,
                shaman = neolithic_human_shaman,
                worker = neolithic_human_worker,
                guard = neolithic_human_guard,
                hunter = neolithic_human_hunter
            },
            worldgen_strength = 2
        },
        raiders = {
            bp_per_turn = 0,
            speed = 1,
            name = "Neolithic Raiding Party",
            sentients = {
                shaman = neolithic_human_shaman,
                hunter = neolithic_human_hunter,
                raider = neolithic_human_raider
            },
            worldgen_strength = 3
        }
    },
    evolves_into = { }
}

function civ_name_gen_neohuman(n)
    n = n % 6;
    if n == 0 then return "{LASTNAME} Tribe";
    elseif n == 1 then return "{LASTNAME} Clan";
    elseif n == 2 then return "{LASTNAME} Gang";
    elseif n == 3 then return "{LASTNAME} Dynasty";
    elseif n == 4 then return "{LASTNAME} Horde";
    elseif n == 5 then return "{LASTNAME} Kindred";
    else return "{LASTNAME} Blood";
    end
end

function leader_name_gen_neohuman(n)
    n = n % 6;
    if n == 0 then return "Lord {FIRSTNAME_M} {LASTNAME}";
    elseif n == 1 then return "Lady {FIRSTNAME_F} {LASTNAME}";
    elseif n == 2 then return "Sir {FIRSTNAME_M} {LASTNAME}";
    elseif n == 3 then return "Dame {FIRSTNAME_F} {LASTNAME}";
    elseif n == 4 then return "Warlord {LASTNAME}";
    elseif n == 5 then return "Highness {LASTNAME}";
    end
end