life_events = {
    -- Births
    birth_normal = { 
        min_age=0, 
        max_age=0, 
        description="born as a healthy, normal baby.", 
        weight = 50,
        modifiers = { },  
    },
    birth_sickly = { 
        min_age=0, 
        max_age=0, 
        description="born as a sickly baby.", 
        weight = 5,
        modifiers = { con = -1 },  
    },
    birth_abandoned = { 
        min_age=0, 
        max_age=0, 
        description="born, abandoned at the hospital, and adopted.", 
        weight = 2,
        modifiers = { cha = -1 },  
    },
    birth_basket = { 
        min_age=0, 
        max_age=0, 
        description="born in unknown circumstances, found in a basket outside a church, and adopted.", 
        weight = 1,
        modifiers = { cha = -2 },  
    },

    -- Infancy
    stay_home = { 
        min_age=0, 
        max_age=5, 
        description="remained home with %PRONOUN% parents.", 
        weight = 50,
        modifiers = { },  
    },
    daycare_normal = { 
        min_age=0, 
        max_age=5, 
        description="attended daycare, and had a normal experience with it.", 
        weight = 40,
        modifiers = { cha=1 },  
    },
    daycare_excellent = { 
        min_age=0, 
        max_age=5, 
        description="attended daycare, and had an excellent experience.", 
        weight = 30,
        modifiers = { str=1, dex=1, con=1, int=1, wis=1, cha=1 },  
    },
    daycare_terrible = { 
        min_age=0, 
        max_age=5, 
        description="attended daycare, and experienced terrible neglect.", 
        weight = 5,
        modifiers = { str=-1, dex=-1, con=1, int=-1, wis=-1, cha=-1 },  
    },
    stay_home_terrible = { 
        min_age=0, 
        max_age=5, 
        description="remained home with %PRONOUN% parents, and experienced terrible neglect.", 
        weight = 5,
        modifiers = { str=-1, dex=-1, int=-1, wis=-1, cha=-1 },  
    },
    infant_sickness = { 
        min_age=0, 
        max_age=5, 
        description="became quite unwell.", 
        weight = 1,
        modifiers = { con=-1 },  
    },
    infant_dropped_on_head = { 
        min_age=0, 
        max_age=5, 
        description="was dropped on their head.", 
        weight = 1,
        modifiers = { int=-1,wis=-1 },  
    },
    infant_shaken = { 
        min_age=0, 
        max_age=5, 
        description="was shaken by an angry parent.", 
        weight = 1,
        modifiers = { con=-1 },  
    },

    -- School age
    school_normal = { 
        min_age=6, 
        max_age=16, 
        description="attended school, and was quite unremarkable.", 
        weight = 60,
        modifiers = { },  
    },
    homeschool_normal = { 
        min_age=6, 
        max_age=16, 
        description="was educated at home, and had a quite normal experience.", 
        weight = 30,
        modifiers = { },  
    },
    school_excellent = { 
        min_age=6, 
        max_age=16, 
        description="attended school, and excelled.", 
        weight = 10,
        modifiers = { str=1, dex=1, con=1, int=1, wis=1, cha=1 },  
    },
    school_awful = { 
        min_age=6, 
        max_age=16, 
        description="attended school, and performed terribly.", 
        weight = 10,
        modifiers = { str=-1, dex=-1, con=-1, int=-1, wis=-1, cha=-1 },  
    },
    school_bully = { 
        min_age=6, 
        max_age=16, 
        description="attended school, and neglected academics in the name of hitting people.", 
        weight = 10,
        modifiers = { str=1, dex=1, con=1, int=-1, wis=-1, cha=-1 },  
    },
    school_bullied = { 
        min_age=6, 
        max_age=16, 
        description="attended school, and did well but was horribly bullied by other students.", 
        weight = 10,
        modifiers = { str=-1, dex=-1, con=-1, int=1, wis=1, cha=1 },  
    },
    school_sports_won = { 
        min_age=6, 
        max_age=16, 
        description="won a sporting event at school.", 
        weight = 10,
        modifiers = { str=1, dex=1 },  
    },
    school_sports_injury = { 
        min_age=6, 
        max_age=16, 
        description="was injured in a sporting event at school.", 
        weight = 10,
        modifiers = { str=-1, dex=-1 },  
    },
    school_sports_truant = { 
        min_age=6, 
        max_age=16, 
        description="missed a lot of school, for fun.", 
        weight = 10,
        modifiers = { int=-1 },  
    },
    school_dance = { 
        min_age=6, 
        max_age=16, 
        description="performed amazingly at the school dance.", 
        weight = 10,
        modifiers = { dex=1,cha=1 },  
    },
    school_poetry = { 
        min_age=6, 
        max_age=16, 
        description="won a poetry contest at school.", 
        weight = 10,
        modifiers = { int=1, cha=1 },  
    },
    school_drugs = { 
        min_age=12, 
        max_age=16, 
        description="had troubles with drugs at school.", 
        weight = 10,
        modifiers = { int=-1 },  
    },
    school_dating = { 
        min_age=12, 
        max_age=16, 
        description="missed a lot of school, due to excessive dating.", 
        weight = 20,
        modifiers = { int=-1, cha=1 },  
    },
    school_military = { 
        min_age=12, 
        max_age=16, 
        description="attended the Eden Guard cadet program.", 
        weight = 10,
        modifiers = { str=1,dex=1,wis=-1 },  
    },
    school_leadership = { 
        min_age=12, 
        max_age=16, 
        description="attended Eden Guard leadership training at school.", 
        weight = 10,
        modifiers = { wis=-1,cha=1,dex=1 },  
    },

    -- Young adulthood
    further_education = { 
        min_age=16, 
        max_age=21, 
        description="attended further education.", 
        weight = 20,
        modifiers = { int=1,wis=1 },  
    },
    vocational_training = { 
        min_age=16, 
        max_age=21, 
        description="attended trade school.", 
        weight = 20,
        modifiers = { str=1,dex=1 },  
    },
    eden_guard = { 
        min_age=16, 
        max_age=99, 
        description="served as a grunt in the Eden Guard.", 
        weight = 20,
        modifiers = { str=1,dex=1,con=1 },  
    },
    eden_guard_squad_leader = { 
        min_age=16, 
        max_age=99, 
        description="served as a squad leader in the Eden Guard.", 
        weight = 20,
        modifiers = { str=1,dex=1,cha=1 },  
    },
    ganger = { 
        min_age=16, 
        max_age=99, 
        description="ran with a gang in the Eden Hive.", 
        weight = 5,
        modifiers = { str=1,dex=1,con=1,int=-1,wis=-1 },  
    },
    gang_leader = { 
        min_age=16, 
        max_age=99, 
        description="lead a gang in the Eden Hive.", 
        weight = 2,
        modifiers = { str=1,dex=1,cha=2,int=-1,wis=-1 },  
    },
    prison = { 
        min_age=16, 
        max_age=99, 
        description="went to prison.", 
        weight = 2,
        modifiers = { str=1,dex=1,int=-1 },  
    },
    cultist = { 
        min_age=16, 
        max_age=99, 
        description="joined a cult.", 
        weight = 2,
        modifiers = { wis=-1,con=1 },  
    },
    cult_leader = { 
        min_age=16, 
        max_age=99, 
        description="lead a cult.", 
        weight = 1,
        modifiers = { wis=-1,cha=1 },  
    },
    mundane = { 
        min_age=16, 
        max_age=99, 
        description="had a mundane year.", 
        weight = 50,
        modifiers = { },  
    },
}